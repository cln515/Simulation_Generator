// SimdataForLiDARCamCalib.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include "basicPly/BasicPly.h"
#include "LRF_emu.h"
#include "image_utility/image_utility.h"
#include "PanoramaRenderer\PanoramaRenderer.h"
#include "LBEmulator.h"
#include "nlohmann/json.hpp"
#include "FusionPlatform.h"
//#include "ladybug.h"
//#include "ladybugImageAdjustment.h"
//#include "ladybuggeom.h"
//#include "ladybugrenderer.h"
//#include <ladybugstream.h>
//#include <ladybuggps.h>
//#include <ladybugvideo.h>

#include <iostream>


int main(int argc, char *argv[])
{

	std::ifstream i(argv[1]);
	nlohmann::json config;
	i >> config;


	string input=config["input"].get<std::string>();
	string cparaf = config["cpara"].get<std::string>();
	string motf = config["motion"].get<std::string>();
	ifstream ifsmot(motf);
	string str;
	vector<Matrix4d> motiondatas;
	vector<double> motiontimes;


	string outputFolder = config["output"].get<std::string>();;
	BasicPly modelData;

	vector<string> refNameList;
	refNameList.push_back(input);

	modelData.readPlyFile_(input);

	camera* pr=new camera();
	pr->setDataRGB(modelData.getVertecesPointer(), modelData.getFaces(), modelData.getRgbaPointer(), modelData.getVertexNumber(), modelData.getFaceNumber());
	pr->setFisheye();
	pr->createContext(1024, 1024);

	int viewHeight;
	int viewWidth;
	PanoramaRenderer::getViewSize(viewWidth, viewHeight);

	//ifstream ifsmot(motf);
	//string str;
	//vector<Matrix4d> motiondatas;
	while (getline(ifsmot, str)) {
		double motdata[9];
		double time;
		for (int i = 0; i < 10; i++) {
			if (i == 0) {
				time = stod(str.substr(0, str.find_first_of(" ")));
			}else if (i != 9) {
				motdata[i-1] = stod(str.substr(0, str.find_first_of(" ")));
			}
			else{
				motdata[i - 1] = stod(str);
			}
			str.erase(0, str.find_first_of(" ") + 1);
			
		}
		Matrix4d m1 = lookat2matrix(motdata);
		motiondatas.push_back(m1);
		cout << m1 << endl;
		motiontimes.push_back(time);
	}
	ifsmot.close();



	//pr.render(motiondatas.at(0));
	//
	//cv::Mat colorimage = cv::Mat(cv::Size(viewWidth, viewHeight), CV_32FC1),us8img;
	//memcpy(colorimage.data, pr.getReflectanceData(), sizeof(float) * colorimage.size().width*colorimage.size().height);
	////for (int i = 0; i < colorimage.size().width*colorimage.size().height; i+= colorimage.size().width/5)std::cout << pr.getReflectanceData()[i] << ",";
	//colorimage.convertTo(us8img, CV_8UC1, 256.0f);
	//cv::imwrite("rfl.png", us8img);


	IntersectionSearcher* isearch = new IntersectionSearcher();
	{
		float* vp = modelData.getVertecesPointer();
		unsigned int* fp = modelData.getFaces();
		isearch->buildTree(vp, fp, modelData.getVertexNumber(), modelData.getFaceNumber());//AABBtreeの作成
		//float* rf = modelData.getReflectancePointer();
		//isearch->setReflectance(rf, modelData.getVertexNumber());
	}

//	int sensortype = config["sensor"].get<int>();
	std::list<nlohmann::json> lidars = config["LiDAR"].get<std::list<nlohmann::json>>();
	LRF::LRF_emulator::sensor_type sensortype;
	nlohmann::json lidarconf =	lidars.front();
	std::string lidType = lidarconf["type"].get<std::string>();
	if (lidType.compare("VLP-16")==0) {
		sensortype = LRF::LRF_emulator::sensor_type::VLP_16;
	}
	else if(lidType.compare("ZF-IMAGER") == 0){
		sensortype = LRF::LRF_emulator::sensor_type::ZF_IMAGER;
	}//custom lidar
	std::cout << "sensor type:" << sensortype <<","<< lidType<< std::endl;
	LRF::LRF_emulator* lidartest= new LRF::LRF_emulator(sensortype);
	lidartest->setIntersectionSearcher(isearch);
	_6dof _ext = { lidarconf["rx"].get<double>()
		, lidarconf["ry"].get<double>()
		, lidarconf["rz"].get<double>()
		, lidarconf["x"].get<double>()
		, lidarconf["y"].get<double>()
		, lidarconf["z"].get<double>() };
	Matrix4d extParam = _6dof2m(_ext);

	FusionPlatform fp;
	pr->fileBase = "outimg";
	fp.cameras.push_back(pr);
	fp.lidars.push_back(lidartest);
	fp.setTime(motiontimes, motiondatas);
	fp.scan(0, 5.0);


    std::cout << "Hello World!\n"; 
}

