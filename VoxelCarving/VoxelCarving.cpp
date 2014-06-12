#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>

using namespace std;
int thresh= 220;
struct Voxel
{
    float x, y, z;
    float depth; // used to decide which camera colours the voxel
    unsigned char r, g, b;
    unsigned char camera_index;
};

void VoxelCarvingPredator();

int main()
{
    VoxelCarvingPredator();

    return 0;
}

void VoxelCarvingPredator()
{
    char line[1024];
    int num;
    vector<cv::Mat> imgs;
    vector<cv::Mat> projections;
    vector<cv::Mat> masks;
    vector<Voxel> voxel_grid;
    cv::Mat X(3, 1, CV_32F);
    cv::Mat X2(4, 1, CV_32F);
    cv::Point3f voxel_origin(-100, -100, -100);

    int xdiv = 200; // voxel discretization
    int ydiv = 200;
    int zdiv = 200;

    float voxel_size = 800 / xdiv; // 800 is the size of the cube

    // Read in projection matrices
    ifstream input("P.txt");
    assert(input);

    input.getline(line, sizeof(line));
    sscanf(line, "%d", &num);

    for(int i=0; i < num; i++) {
        cv::Mat P(3, 4, CV_32F);
        input.getline(line, sizeof(line)); // blank

        for(int j=0; j < 3; j++) {
            input.getline(line, sizeof(line));
            stringstream a(line);

            a >> P.at<float>(j, 0);
            a >> P.at<float>(j, 1);
            a >> P.at<float>(j, 2);
            a >> P.at<float>(j, 3);
        }

        projections.push_back(P);
    }


    // Init grid
    voxel_grid.resize(xdiv * ydiv * zdiv);

    int k = 0;
    for(int x=0; x < xdiv; x++) {
        for(int y=0; y < ydiv; y++) {
            for(int z=0; z < zdiv; z++) {
                voxel_grid[k].x = voxel_origin.x + x*voxel_size;
                voxel_grid[k].y = voxel_origin.y + y*voxel_size;
                voxel_grid[k].z = voxel_origin.z + z*voxel_size;
                voxel_grid[k].depth = FLT_MAX;

                k++;
            }
        }
    }

    for(size_t k=0; k < projections.size(); k++) {
        char str[128];
        cv::Mat img, mask;
        vector<Voxel> visible_voxels;

        sprintf(str, "%02d.jpg", (int)k);
        cout << "Loading " <<  str << endl;

        img = cv::imread(string("visualize/") + str); assert(img.data);
        mask = cv::Mat::ones(img.size(), CV_8U) * 255;

        // Background removal, white background so it's easy
        for(int y=0; y < mask.rows; y++) {
            for(int x=0; x < mask.cols; x++) {
                if(img.at<cv::Vec3b>(y,x)[0] > thresh && img.at<cv::Vec3b>(y,x)[1] > thresh && img.at<cv::Vec3b>(y,x)[2] > thresh) {
                    mask.at<uchar>(y,x) = 0;
                }
            }
        }

        // Fill up any holes
        dilate(mask, mask, cv::Mat());

        imgs.push_back(img);
        masks.push_back(mask);

        // Visualization
        {
            cv::Mat img2, mask2;

            cv::resize(img, img2, cv::Size(img.cols/2, img.rows/2));
            cv::resize(mask, mask2, cv::Size(img.cols/2, img.rows/2));

            cv::cvtColor(mask2, mask2, cv::COLOR_GRAY2BGR);

            cv::Mat canvas(img2.rows, img2.cols*2, CV_8UC3);

            img2.copyTo(canvas(cv::Rect(0, 0, img2.cols, img2.rows)));
            mask2.copyTo(canvas(cv::Rect(img2.cols, 0, img2.cols, img2.rows)));

            cv::imshow("main", canvas);

            cv::waitKey(20);
        }

        cv::Mat &P = projections[k];

        // Visibility testing
        for(size_t i=0; i < voxel_grid.size(); i++) {
            Voxel &vox = voxel_grid[i];

            X2.at<float>(0) = vox.x;
            X2.at<float>(1) = vox.y;
            X2.at<float>(2) = vox.z;
            X2.at<float>(3) = 1.0;

            X = P * X2;

            int x = X.at<float>(0) / X.at<float>(2);
            int y = X.at<float>(1) / X.at<float>(2);

            if(x < 0 || x >= mask.cols || y < 0 || y >= mask.rows) {
                continue;
            }

            if(mask.at<uchar>(y,x)) {
                if(X.at<float>(2) < vox.depth) {
                    vox.depth = X.at<float>(2);

                    vox.r = img.at<cv::Vec3b>(y,x)[2];
                    vox.g = img.at<cv::Vec3b>(y,x)[1];
                    vox.b = img.at<cv::Vec3b>(y,x)[0];

                    vox.camera_index = k;
                }

                visible_voxels.push_back(vox);
            }
        }

        voxel_grid = visible_voxels;
    }

    cout << "Refining voxels ... " << endl;

    // Shrink the voxel size
    int sub_division = 2;
    voxel_size /= sub_division;

    vector<Voxel> new_voxels;

    for(size_t i=0; i < voxel_grid.size(); i++) {
        for(int x=1; x < sub_division; x++) {
            for(int y=1; y < sub_division; y++) {
                for(int z=1; z < sub_division; z++) {
                    Voxel v = voxel_grid[i];

                    v.x = v.x + x*voxel_size;
                    v.y = v.y + y*voxel_size;
                    v.z = v.z + z*voxel_size;

                    v.depth = FLT_MAX;

                    new_voxels.push_back(v);
                }
            }
        }
    }

    // Repeat visibility test, probably should put this as a function
    for(size_t k=0; k < projections.size(); k++) {
        vector<Voxel> visible_voxels;

        cv::Mat &img = imgs[k];
        cv::Mat &mask = masks[k];
        cv::Mat &P = projections[k];

        for(size_t i=0; i < new_voxels.size(); i++) {
            Voxel &vox = new_voxels[i];

            X2.at<float>(0) = vox.x;
            X2.at<float>(1) = vox.y;
            X2.at<float>(2) = vox.z;
            X2.at<float>(3) = 1.0;

            X = P * X2;

            int x = X.at<float>(0) / X.at<float>(2);
            int y = X.at<float>(1) / X.at<float>(2);

            if(x < 0 || x >= mask.cols || y < 0 || y >= mask.rows) {
                continue;
            }

            if(mask.at<uchar>(y,x)) {
                if(X.at<float>(2) < vox.depth) {
                    vox.depth = X.at<float>(2);

                    vox.r = img.at<cv::Vec3b>(y,x)[2];
                    vox.g = img.at<cv::Vec3b>(y,x)[1];
                    vox.b = img.at<cv::Vec3b>(y,x)[0];

                    vox.camera_index = k;
                }

                visible_voxels.push_back(vox);
            }
        }

        new_voxels = visible_voxels;
    }

    // Output to Meshlab readable file
    cout << "Saving to output.ply" << endl;

    ofstream output("output.ply");

    output << "ply" << endl;
    output << "format ascii 1.0" << endl;
    output << "element vertex " << voxel_grid.size() + new_voxels.size() << endl;
    output << "property float x" << endl;
    output << "property float y" << endl;
    output << "property float z" << endl;
    output << "property uchar diffuse_red" << endl;
    output << "property uchar diffuse_green" << endl;
    output << "property uchar diffuse_blue" << endl;
    output << "end_header" << endl;

    for(size_t i=0; i < voxel_grid.size(); i++) {
        Voxel &v = voxel_grid[i];
        output << v.x << " " << v.y << " " << v.z << " " << (int)v.r << " " << (int)v.g << " " << (int)v.b << endl;
    }

    // Refined voxels
    for(size_t i=0; i < new_voxels.size(); i++) {
        Voxel &v = new_voxels[i];
        output << v.x << " " << v.y << " " << v.z << " " << (int)v.r << " " << (int)v.g << " " << (int)v.b << endl;
    }
}
