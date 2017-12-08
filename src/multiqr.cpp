#include <iostream>
#include <dirent.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <zbar.h>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "../include/qrgen/QrCode.hpp"
#include "../include/Base64.h"

using namespace std;
using namespace qrcodegen;
using namespace zbar;

/*
 * Chunk of file data with costom header to be encoded to QR code
 */
class QrChunk {
  const int m_raw_header_size = 5;
  const int m_header_size = 8;
  string m_header; // in base64 format
  string m_body; // in base64 format

  public:
    // Deffault constructor
    QrChunk() {}

    QrChunk(string data) {
      m_header = data.substr(0, m_header_size);
      m_body = data.substr(m_header_size, data.size() - m_header_size);
    }

    string getHeader() { return m_header; }
  
    string getBody() { return m_body; }
  
    string getData() { return m_header + m_body; }
  
    void setBody(string data) { m_body = data; }
  
    void setHeader(int index, int total) {
      char header[m_raw_header_size];
      snprintf(header, sizeof(header), "%02d%02d", index, total);
      m_header = base64_encode((unsigned char*)header, m_raw_header_size);
    }
};

/*
 * Encode a binary files into multiple QR codes
 */
class QrEncoder {
  string m_data;
  int m_border_scale = 4;
  int m_out_qr_scale = 4;

  vector<QrCode> m_qrs;
  vector<QrChunk> m_chunks;

  void split_to_chunks(int chunk_size=738) {

    string data64 = get_data64();

    int num_chunks = ceil(data64.size() / (double)chunk_size);

    for (int i = 0; i < num_chunks; i++) {
      int start = ((i + 1) * chunk_size) - chunk_size;
      if (start + chunk_size > data64.size()) {
        chunk_size = data64.size() - (start-1);
      }

      QrChunk chunk;
      chunk.setBody(data64.substr(start, chunk_size));
      chunk.setHeader(i, num_chunks);

      m_chunks.push_back(chunk);
    }
  }

  public:

    int from_file(string file_path) {
      stringstream str_stream;
      
      ifstream f (file_path);
      if (! f.is_open()) {
        cout << "Can not open file!" << endl;
        return -1;
      }

      // Read file stream to the string buffer
      str_stream << f.rdbuf();
      m_data = str_stream.str();
      
      f.close();

      return 0;
    }

    int encode (int chunk_size) {
      
      split_to_chunks();

      for (QrChunk chunk : m_chunks) {
        QrCode qr = QrCode::encodeText(chunk.getData().c_str(), QrCode::Ecc::LOW);
        m_qrs.push_back(qr);
      }
      return 0;
    }

    vector<cv::Mat> get_imgs() {
      vector<cv::Mat> mats;
      for (auto qr : m_qrs) {
        mats.push_back(qr_to_img(qr));
      }
      return mats;
    }

    cv::Mat qr_to_img(QrCode qr) {
      int qr_size = qr.getSize();

      cv::Mat qr_img (qr_size, qr_size, CV_8UC1);

      for (int y = 0; y < qr_size; y++) {
        for (int x = 0; x < qr_size; x++) {
          qr_img.at<int>(y, x, 0) = (qr.getModule(x, y) ^ 1) * 255;
        };
      }
      
      return qr_img;
    }

    void save_as_imgs(string output_dir, string image_format="png") {
      int index = 0;
      for (auto qr: m_qrs) {
        cout << index << endl;
        int qr_size = qr.getSize();

        cv::Mat qr_img = qr_to_img(qr);
        cv::Mat qr_img_resized (qr_size, qr_size, CV_8UC1);
        cv::Mat qr_img_bordered (qr_size, qr_size, CV_8UC1);

        // Add border
        cv::copyMakeBorder(qr_img, qr_img_bordered, m_border_scale, m_border_scale, m_border_scale, m_border_scale, cv::BORDER_CONSTANT, 255);

        // Scale image 5 times
        cv::resize(qr_img_bordered, qr_img_resized, cv::Size(), m_out_qr_scale, m_out_qr_scale, cv::INTER_NEAREST);

        cv::imwrite(output_dir + "/" + to_string(index) + "." + image_format, qr_img_resized);
        index++;
      }
    }

    void save_as_svgs (string output_dir) {
      int index = 0;
      for (auto qr: m_qrs) {
        std::string svg = qr.toSvgString(m_border_scale);
        ofstream outputFile(output_dir + "/" + to_string(index) + ".svg");
        outputFile << svg;
        outputFile.close();
        index++;
      }
    }
};

/*
 * Decode binary file from multiple qr codes
 */
class QrDecoder {
  int m_num_files = 16;
  string m_input_folder;
  string m_data64;

  vector<cv::Mat> m_images;
  vector<QrChunk> m_chunks;

  public:

    void read_images(string dir_path) {
      for (int i = 0; i < m_num_files; i++) {
        cv::Mat img = cv::imread(dir_path + "/" + to_string(i) + ".png");
        m_images.push_back(img);
      }
    }

    void decode_images() {
      ImageScanner scanner;

      // Configure the reader
      scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);

      int total;

      for (auto img : m_images) {
        cv::Mat img_gray, img_resized;

        // Convert to grayscale
        cvtColor(img, img_gray, CV_RGBA2GRAY);

        int width = img_gray.cols;
        int height = img_gray.rows;

        // Prep data for qr image decoder
        uchar *raw = (uchar *)(img_gray.data);

        Image image(width, height, "Y800", raw, width * height);

        // Scan the image for barcodes
        int result = scanner.scan(image);

        if (result <= 0) {
          cout << "Can not read QR code " << endl;
          cv::imshow("Error", img_gray);
          cv::waitKey(0);
        }

        string data;
        for (auto symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
          data = data + symbol->get_data();          
        }

        QrChunk chunk(data);
        m_chunks.push_back(chunk);
      }
    }

    string get_data() {
      string data;
      for (QrChunk chunk : m_chunks) {
        cout <<  base64_decode(chunk.getHeader()) << endl;
        data += chunk.getBody();
      }
      return base64_decode(data);
    }

    int save_file(string file_name) {
      string data = get_data();

      ofstream outputFile(file_name);
      if (! outputFile.is_open()) {
        cout << "Can not write to " << file_name << endl;
        return -1;
      }
      outputFile << data;
      outputFile.close();
      return 0;
    }
};

int main(int argc, char **argv) {

  if (argc < 2) {
    cout << "Usage: multiqr <encode|decode>" << endl;
    return 0;
  }

  if (string(argv[1]) == "encode") {

    if (argc < 4) {
      cout << "Usage: encode <input_file> <output_dir>" << endl;
      return 0;
    }

    string input_file = argv[2];
    string output_folder = argv[3];
    
    cout << "Encoding file " << input_file << endl;

    QrEncoder encoder;
    encoder.from_file(input_file);
    encoder.encode(738);
    encoder.save_as_imgs(output_folder);
  
  } else if (string(argv[1]) == "decode") {

    if (argc < 4) {
      cout << "Usage: decode <input_dir> <output_file>" << endl;
      return 0;
    }

    string input_dir = argv[2];
    string output_file = argv[3];

    cout << "Decoding files " << input_dir << endl;

    QrDecoder decoder;  
    decoder.read_images(input_dir);
    decoder.decode_images();
    decoder.save_file(output_file);
  }

  return 0;
}