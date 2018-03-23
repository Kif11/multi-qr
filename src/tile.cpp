#include <iostream>

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

int main(int argc, char** argv) {

  if (argc < 3) {
    cout << "Usage: tile <folder_with_qrs> <output_image>" << endl;
    return 0;
  }

  string qr_folder = argv[1];
  string out_image = argv[2];
  int border_scale = 20;
  Mat img_bordered;
  
  ImageCells cells(4, 5, 400, 400); 
  Mat img;

  int count = 0;
  for (int i = 0; i < cells.rows(); i++) {
    for (int j = 0; j < cells.cols(); j++) {

      if (count >= 19) {
        break;
      }
      string image_path = qr_folder + "/" + to_string(count) + ".png";      

      img = imread(image_path);
      cells.setCell(j, i, img); // here you see how to use  setCell
      
      count++;
    }
  }

  // imshow("cells.image", cells.image);
  // waitKey();

  cv::copyMakeBorder(cells.image, img_bordered, border_scale, border_scale, border_scale, border_scale, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

  imwrite(out_image, img_bordered);
      
  return 0;
}