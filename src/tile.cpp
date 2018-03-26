#include <iostream>
#include <dirent.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

class ImageCells {
  public:
    ImageCells(int rows, int cols, int width, int height);
    virtual ~ImageCells() {}

    int cellwidth() const { return m_width; }
    int cellheight() const { return m_height; }
    int cols() const { return m_cols; }
    int rows() const { return m_rows; }

    void setCell( int col, int row, Mat img );
    void setImage( Mat img );
    Mat getCell( int col, int row );
    Mat image;

  protected:
    int m_width;
    int m_height;
    int m_cols;
    int m_rows;
};

ImageCells::ImageCells( int rows, int cols, int width, int height) {
  image = Mat::zeros(rows * height, cols * width, CV_8UC3);
  image.setTo(cv::Scalar(255, 255, 255)); // Set image to white
  m_width = width;
  m_height = height;
  m_cols = cols;
  m_rows = rows;
}

void ImageCells::setCell( int col, int row, Mat img ) {
  if(img.cols == m_width & img.rows == m_height) {
    Mat roi = image( Rect(col * m_width, row * m_height, m_width, m_height) );
    img.copyTo(roi);
  }
}

Mat ImageCells::getCell( int col, int row ) {
  Mat roi = image( Rect(col * m_width, row * m_height, m_width, m_height) );
  return roi.clone();
}

void ImageCells::setImage( Mat img ) {
  if (img.cols >= image.cols & img.rows >= image.rows) {
    img.copyTo(image);    
  }
}

int count_files(const char* directory) {
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(directory)) != NULL) {
    /* print all the files and directories within directory */
    int count = 0;
    while ((ent = readdir(dir)) != NULL) {
      if (!strncmp (ent->d_name, ".", 1)) {
        continue;        
      }
      count++;
    }
    closedir (dir);
    return count;
  } else {
    /* could not open directory */
    perror ("Could not open directory");
    return -1;
  }
}

int main(int argc, char** argv) {

  if (argc < 3) {
    cout << "Usage: tile <folder_with_qrs> <output_folder> [footer_path]" << endl;
    return 0;
  }

  string qr_folder = argv[1];
  string out_folder = argv[2];
  
  string footer_path = "";
  bool is_footer = false;
  if (argc == 4) {
    footer_path = argv[3];
    is_footer = true;
  }

  int qrs_on_page = 20;
  int qr_size = 400;
  int total_qrs = count_files(qr_folder.c_str());  
  int num_pages = ceil(total_qrs / (double)qrs_on_page);
  int border_scale = 40;

  int qr_counter = 0;

  Mat footer_img;
  // make square with text
  if (is_footer) {
    footer_img = imread(footer_path); 
    // imshow("text", footer_img);
    // waitKey(); 
  }

  for (int p = 0; p < num_pages; p++) { // for every page

    ImageCells cells(4, 5, qr_size, qr_size);

    Mat img;
    Mat img_bordered;  

    int per_page_counter = 0;
    bool done = false;
    for (int i = 0; i < cells.rows(); i++) {
      if (done) break;
      for (int j = 0; j < cells.cols(); j++) {

        if (per_page_counter >= qrs_on_page ) {
          // break and possibly go to the next page
          // qrs_on_page QRs per page + one empty square for text
          break;
        }
        string image_path = qr_folder + "/" + to_string(qr_counter) + ".png";   

        img = imread(image_path);

        cells.setCell(j, i, img); // here you see how to use  setCell
        
        per_page_counter++;
        qr_counter++;

        if (img.empty()) {
          // If image doesn't exist
          if (is_footer) {
            cells.setCell(j, i, footer_img);
          }
          done = true;
          break;
        }
      }
    }

    cv::copyMakeBorder(cells.image, img_bordered, border_scale, border_scale, border_scale, border_scale, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    imwrite(out_folder + "/" + to_string(p) + ".png", img_bordered);

  }

  // imshow("cells.image", cells.image);
  // waitKey();    
  return 0;
}