#include "stdafx.h"

#include <core/logger.h>
#include <core/frame.h>
#include <core/frame_reader_yaml.h>
#include <core/cloud_colorer.h>

#include <cstdlib>
#include <core/visualization_2d.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/visualization/pcl_visualizer.h>

#include "PlaneCalculator.h"

int main(int argc, char** argv)
{

	std::vector<std::string> frames = { "frame_Ensenso_1545410136703379.yaml", "frame_Ensenso_1545410193355854.yaml", "frame_Ensenso_1545410240542914.yaml",
									"frame_Ensenso_1545410305811152.yaml", "frame_Ensenso_1545410407922793.yaml", "frame_Photoneo_1545410589140931.yaml" };

	std::vector<std::string> frames2 = { "frame_Ensenso_1545405219052414.yaml", "frame_Ensenso_1545405279188999.yaml", "frame_Ensenso_1545405306830019.yaml",
								  "frame_Ensenso_1545405336821894.yaml", "frame_Ensenso_1545405339366027.yaml", "frame_Ensenso_1545405393874214.yaml",
								  "frame_Ensenso_1545405438794180.yaml", "frame_Ensenso_1545405513323357.yaml", "frame_Ensenso_1545405589881548.yaml", "frame_Ensenso_1545405592416104.yaml" };


	for (const std::string& next_frame : frames2) {
		const cvs::core::Frame::Ptr frame = cvs::core::FrameReaderYAML::read("plane/" + next_frame);
		if (!frame)
		{
			cvs::core::Logger::logMessage(cvs::core::SeverityLevel::Error) <<
				"Can't find test frame. If it is ypur firs time, make sure Wirking Dir is set to \"$(SolutionDir)/../../data/$(Configuration)/\" ";
			system("pause");
			return -1;
		}



		const cv::Mat frameImage = cvs::core::Visualization2D::draw(frame->m_intensityImage, 1, "amplitude is just an IR-picture (float)", false, cv::COLORMAP_BONE, cv::noArray(), true);
		cvs::core::Visualization2D::show(frameImage, "amplitude");

		/*std::vector<cv::Mat> frameCloudChannels;
		cv::split(frame->m_pointMatrix, frameCloudChannels);
		const cv::Mat frameDepth = cvs::core::Visualization2D::draw(frameCloudChannels.back(), 1, "'depth' is the 2nd channel of a cloud - it has come artifacts", false, cv::COLORMAP_HOT);
		cvs::core::Visualization2D::show(frameDepth, "depth");*/


		boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer{ new pcl::visualization::PCLVisualizer("3D Viewer") };
		const pcl::PointCloud<pcl::PointXYZRGBA>::Ptr frameCloud = cvs::core::CloudColorer::copyCloudAsColored(frame->m_pointMatrix, frame->m_validMask, frame->m_intensityImage);
		viewer->addPointCloud<pcl::PointXYZRGBA>(frameCloud, "frameCloud", 0);

		pcl::PointXYZRGBA min_by_x_point;
		float min_x = -INFINITY;

		std::vector<double> x_vals; x_vals.resize(frameCloud->size());
		std::vector<double> y_vals; y_vals.resize(frameCloud->size());
		std::vector<double> z_vals; z_vals.resize(frameCloud->size());

		int i = 0;
		for (auto it = frameCloud->begin(); it != frameCloud->end(); ++it) {
			x_vals[i] = it->x;
			y_vals[i] = it->y;
			z_vals[i] = it->z;
			++i;
			if (it->x > min_x) {
				min_x = it->x;
				min_by_x_point = *it;
			}
		}

		std::nth_element(x_vals.begin(), x_vals.begin() + frameCloud->size() / 2, x_vals.end());
		float x_median = *std::next(x_vals.begin(), frameCloud->size() / 2);

		std::nth_element(y_vals.begin(), y_vals.begin() + frameCloud->size() / 2, y_vals.end());
		float y_median = *std::next(y_vals.begin(), frameCloud->size() / 2);

		std::nth_element(z_vals.begin(), z_vals.begin() + frameCloud->size() / 2, z_vals.end());
		float z_median = *std::next(z_vals.begin(), frameCloud->size() / 2);

		auto plane = PlaneCalculator::calculatePlaneHintHoles(frame->m_pointMatrix, frame->m_intensityImage);

		//plane->values[0] = 1; plane->values[1] = 0; plane->values[2] = 0; plane->values[3] = 0;


		pcl::PointXYZ norm_vector_start_point;

		norm_vector_start_point.x = x_median;
		norm_vector_start_point.y = y_median;
		norm_vector_start_point.z = z_median;


		double dist_max = 0;

		for (size_t i = 0; i < frameCloud->size(); ++i) {
			auto frameCloud_i = frameCloud->at(i);


			double dist_x = (frameCloud_i.x - norm_vector_start_point.x);
			double dist_y = (frameCloud_i.y - norm_vector_start_point.y);
			double dist_z = (frameCloud_i.z - norm_vector_start_point.z);

			double dist_all = sqrt(dist_x * dist_x + dist_y * dist_y + dist_z * dist_z);

			if (dist_all > dist_max) {
				dist_max = dist_all;
			}
		}

		double norm_vector_length = sqrt(plane->values[0] * plane->values[0] + plane->values[1] * plane->values[1] + plane->values[2] * plane->values[2]);

		if ( pow(x_median + plane->values[0], 2) + pow(y_median + plane->values[1], 2) + pow(z_median + plane->values[2], 2) > 
			pow(x_median, 2) + pow(y_median, 2) + pow(z_median, 2) ) 
		{
			plane->values[0] *= -1;
			plane->values[1] *= -1;
			plane->values[2] *= -1;
			plane->values[3] *= -1;
		} // change orientation to camera

		pcl::PointXYZ norm_vector_end_point;

		norm_vector_end_point.x = x_median + dist_max * plane->values[0] / norm_vector_length;
		norm_vector_end_point.y = y_median + dist_max * plane->values[1] / norm_vector_length;
		norm_vector_end_point.z = z_median + dist_max * plane->values[2] / norm_vector_length;

		std::cout << "Plane normal vector - x:" << plane->values[0] << " y: " << plane->values[1] << " z: " << plane->values[2] << std::endl;


		viewer->addLine<pcl::PointXYZ, pcl::PointXYZ>(norm_vector_start_point, norm_vector_end_point, 0.0, 1.0, 0.0);


		boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer2{ new pcl::visualization::PCLVisualizer("3D Viewer") };
		auto cloudDistanceColored = PlaneCalculator::calculateDistanceToPlane(frame->m_pointMatrix, plane);
		viewer2->addPointCloud<pcl::PointXYZRGBA>(cloudDistanceColored, "frameCloudDistance", 0);


		cvs::core::Logger::logMessage(cvs::core::SeverityLevel::Warning) << "Log is automatically saved to \"cvs.log\" - don't be shy to use it.";
		cvs::core::Logger::logMessage(cvs::core::SeverityLevel::Info) << "You may use mouse anr hotkeys (press [h] for help). Close visializer to exit.";

		while (!viewer->wasStopped())
		{
			viewer->spinOnce(100);
			boost::this_thread::sleep(boost::posix_time::microseconds(100000));
			viewer2->spinOnce(100);
		}

		viewer2->close();
	}


	return 0;
}
