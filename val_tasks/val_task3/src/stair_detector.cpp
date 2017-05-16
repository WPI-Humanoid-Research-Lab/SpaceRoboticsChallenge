#include "val_task3/stair_detector.h"
#include <visualization_msgs/Marker.h>
#include "val_common/val_common_names.h"
#include <pcl/common/centroid.h>
#include <thread>

#define DISABLE_DRAWINGS true
//#define DISABLE_TRACKBAR true

stair_detector::stair_detector(ros::NodeHandle nh) : nh_(nh), ms_sensor_(nh_), organizedCloud_(new src_perception::StereoPointCloudColor)
{
    ms_sensor_.giveQMatrix(qMatrix_);
    marker_pub_ = nh_.advertise<visualization_msgs::MarkerArray>("detected_stair",1);
}

void stair_detector::showImage(cv::Mat image, std::string caption)
{
#ifdef DISABLE_DRAWINGS
    return;
#endif
    cv::namedWindow( caption, cv::WINDOW_AUTOSIZE );
    cv::imshow( caption, image);
    cv::waitKey(3);
}

void stair_detector::colorSegment(cv::Mat &imgHSV, cv::Mat &outImg)
{
    cv::cvtColor(imgHSV, imgHSV, cv::COLOR_BGR2HSV);
    cv::inRange(imgHSV, cv::Scalar(hsv_[0], hsv_[2], hsv_[4]), cv::Scalar(hsv_[1], hsv_[3], hsv_[5]), outImg);
    //cv::morphologyEx(outImg, outImg, cv::MORPH_OPEN, getStructuringElement( cv::MORPH_ELLIPSE,cv::Size(3,3)));
    //cv::dilate(outImg, outImg, getStructuringElement( cv::MORPH_ELLIPSE, cv::Size(5,5)));
#ifdef DISABLE_TRACKBAR
    return;
#endif
    setTrackbar();
    //ROS_INFO_STREAM(outImg);
    cv::imshow("binary thresholding", outImg);
    cv::imshow("Actual Image", current_image_);
    cv::waitKey(3);
}

size_t stair_detector::findMaxContour(const std::vector<std::vector<cv::Point> >& contours)
{
    int largest_area = 0;
    int largest_contour_index = 0;
    std::vector<std::vector<cv::Point>> hull(contours.size());

    for( size_t i = 0; i< contours.size(); i++ )
    {
        cv::convexHull(contours[i], hull[i], false);
        //  Find the area of contour
        double area = cv::contourArea( hull[i]);

        if(area > largest_area)
        {
            largest_area = area;
            // Store the index of largest contour
            largest_contour_index = i;
        }
    }

    convexHulls_ = hull[largest_contour_index];

    return largest_contour_index;
}

bool stair_detector::getStairLocation(geometry_msgs::Point& stairLoc)
{
    bool foundStair = false;
    src_perception::StereoPointCloudColor::Ptr organizedCloud(new src_perception::StereoPointCloudColor);
    src_perception::PointCloudHelper::generateOrganizedRGBDCloud(current_disparity_, current_image_, qMatrix_, organizedCloud);
    tf::TransformListener listener;
    geometry_msgs::PointStamped geom_point;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::Mat imgHSV, outImg;
    //cv::Point point;

    cv::cvtColor(current_image_, imgHSV, cv::COLOR_BGR2HSV);
    colorSegment(imgHSV, outImg);

    // Find contours
    cv::findContours(outImg, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

    Eigen::Vector4f cloudCentroid;
    if(!contours.empty()) //avoid seg fault at runtime by checking that the contours exist
    {
        findMaxContour(contours);
        foundStair = true;


        cv::Mat hullPoints = cv::Mat::zeros(current_image_.size(), CV_8UC1);
        cv::fillConvexPoly(hullPoints, convexHulls_,cv::Scalar(255));
        cv::Mat nonZeroCoordinates;
        cv::findNonZero(hullPoints, nonZeroCoordinates);
        if (nonZeroCoordinates.total() < 10){
            return false;
        }

        pcl::PointCloud<pcl::PointXYZRGB> currentDetectionCloud;

        for (int k = 0; k < nonZeroCoordinates.total(); k++ ) {
            pcl::PointXYZRGB temp_pclPoint = organizedCloud->at(nonZeroCoordinates.at<cv::Point>(k).x, nonZeroCoordinates.at<cv::Point>(k).y);

            if (temp_pclPoint.z > -2.0)
            {
                currentDetectionCloud.push_back(pcl::PointXYZRGB(temp_pclPoint));

            }
        }


        //  Calculating the Centroid of the handle Point cloud
        pcl::compute3DCentroid(currentDetectionCloud, cloudCentroid);
    }
    if( foundStair )
    {
        geom_point.point.x = cloudCentroid(0);
        geom_point.point.y = cloudCentroid(1);
        geom_point.point.z = cloudCentroid(2);
        geom_point.header.frame_id = VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF;
        try
        {
            listener.waitForTransform(VAL_COMMON_NAMES::WORLD_TF, VAL_COMMON_NAMES::LEFT_CAMERA_OPTICAL_FRAME_TF, ros::Time(0), ros::Duration(3.0));
            listener.transformPoint(VAL_COMMON_NAMES::WORLD_TF, geom_point, geom_point);
        }
        catch (tf::TransformException ex){
            ROS_ERROR("%s",ex.what());
            return false;
        }
    }
    stairLoc.x = double (geom_point.point.x);
    stairLoc.y = double (geom_point.point.y);
    stairLoc.z = double (geom_point.point.z);

    visualize_point(geom_point.point);

    marker_pub_.publish(markers_);

    return foundStair;
}



bool stair_detector::findStair(geometry_msgs::Point& stairLoc)
{
    //VISUALIZATION - include ros::spinOnce();

    cv::Mat outImg;
    ros::spinOnce();
    markers_.markers.clear();
    if(ms_sensor_.giveImage(current_image_))
    {
        if( ms_sensor_.giveDisparityImage(current_disparity_))
        {
            colorSegment(current_image_, outImg);
            //return getStairLocation(stairLoc);
            return true;
        }
    }
    return 0;
}


void stair_detector::setTrackbar()
{
    cv::namedWindow("binary thresholding");
    cv::createTrackbar("hl", "binary thresholding", &hsv_[0], 180);
    cv::createTrackbar("hu", "binary thresholding", &hsv_[1], 180);
    cv::createTrackbar("sl", "binary thresholding", &hsv_[2], 255);
    cv::createTrackbar("su", "binary thresholding", &hsv_[3], 255);
    cv::createTrackbar("vl", "binary thresholding", &hsv_[4], 255);
    cv::createTrackbar("vu", "binary thresholding", &hsv_[5], 255);

    cv::getTrackbarPos("hl", "binary thresholding");
    cv::getTrackbarPos("hu", "binary thresholding");
    cv::getTrackbarPos("sl", "binary thresholding");
    cv::getTrackbarPos("su", "binary thresholding");
    cv::getTrackbarPos("vl", "binary thresholding");
    cv::getTrackbarPos("vu", "binary thresholding");
}

void stair_detector::visualize_point(geometry_msgs::Point point){

    std::cout<< "goal origin :\n"<< point << std::endl;
    static int id = 0;
    visualization_msgs::Marker marker;
    // Set the frame ID and timestamp.  See the TF tutorials for information on these.
    marker.header.frame_id = VAL_COMMON_NAMES::WORLD_TF;
    marker.header.stamp = ros::Time::now();

    // Set the namespace and id for this marker.  This serves to create a unique ID
    // Any marker sent with the same namespace and id will overwrite the old one
    marker.ns = std::to_string(frameID_);
    marker.id = id++;

    // Set the marker type.  Initially this is CUBE, and cycles between that and SPHERE, ARROW, and CYLINDER
    marker.type = visualization_msgs::Marker::CUBE;

    // Set the marker action.  Options are ADD, DELETE, and new in ROS Indigo: 3 (DELETEALL)
    marker.action = visualization_msgs::Marker::ADD;

    // Set the pose of the marker.  This is a full 6DOF pose relative to the frame/time specified in the header
    marker.pose.position = point;
    marker.pose.orientation.w = 1.0f;

    marker.scale.x = 0.01;
    marker.scale.y = 0.01;
    marker.scale.z = 0.01;
    marker.color.a = 1.0; // Don't forget to set the alpha!
    marker.color.r = 0.0;
    marker.color.g = 1.0;
    marker.color.b = 0.0;

    markers_.markers.push_back(marker);
}

stair_detector::~stair_detector()
{

}
