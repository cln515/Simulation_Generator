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
	string motf = config["motion"].get<std::string>();
	std::list<nlohmann::json> cameras = config["Camera"].get<std::list<nlohmann::json>>();
	std::list<nlohmann::json> lidars = config["LiDAR"].get<std::list<nlohmann::json>>();
	double scan_start = config["scan_start"];
	double scan_end = config["scan_end"];
	string outputFolder = config["output"].get<std::string>();;

	ifstream ifsmot(motf);
	string str;
	vector<Matrix4d> motiondatas;
	vector<double> motiontimes;


//ply file loading
	BasicPly modelData;
	modelData.readPlyFile_(input);


	//set sensors
	FusionPlatform fp;
	//set camera
	std::list<nlohmann::json>::iterator cameraconf;	 
	for (cameraconf = cameras.begin(); cameraconf != cameras.end(); cameraconf++) {
		camera* pr = new camera();
		pr->setDataRGB(modelData.getVertecesPointer(), modelData.getFaces(), modelData.getRgbaPointer(), modelData.getVertexNumber(), modelData.getFaceNumber());
		std::string camType = (*cameraconf)["type"].get<std::string>();
		if (camType.compare("Fisheye") == 0) {
			pr->setFisheye();
			pr->setUniqueViewSize((*cameraconf)["width"].get<int>(), (*cameraconf)["height"].get<int>());
		}
		else if (camType.compare("Panorama") == 0) {
			pr->setUniqueViewSize((*cameraconf)["width"].get<int>(), (*cameraconf)["height"].get<int>());
		}
		else {
			pr->setPersRender((*cameraconf)["width"].get<int>() / 2, (*cameraconf)["height"].get<int>() / 2, (*cameraconf)["focal"].get<int>(), (*cameraconf)["focal"].get<int>());//Todo set from input file
			pr->setUniqueViewSize((*cameraconf)["width"].get<int>(), (*cameraconf)["height"].get<int>());
		}
		pr->fileBase = "outimg";

		_6dof _ext = { (*cameraconf)["rx"].get<double>()
		, (*cameraconf)["ry"].get<double>()
		, (*cameraconf)["rz"].get<double>()
		, (*cameraconf)["x"].get<double>()
		, (*cameraconf)["y"].get<double>()
		, (*cameraconf)["z"].get<double>() };
		Matrix4d extParam = _6dof2m(_ext);

		fp.cameras.push_back(pr);
		fp.ext_camera.push_back(extParam);
	}
	//set lidar
	IntersectionSearcher* isearch = new IntersectionSearcher();
	{
		float* vp = modelData.getVertecesPointer();
		unsigned int* fp = modelData.getFaces();
		isearch->buildTree(vp, fp, modelData.getVertexNumber(), modelData.getFaceNumber());//AABBtreeの作成
	}
	
	std::list<nlohmann::json>::iterator lidarconf;
	for (lidarconf = lidars.begin(); lidarconf != lidars.end(); lidarconf++) {
		LRF::LRF_emulator::sensor_type sensortype;
		std::string lidType =(*lidarconf)["type"].get<std::string>();
		if (lidType.compare("VLP-16") == 0) {
			sensortype = LRF::LRF_emulator::sensor_type::VLP_16;
		}
		else if (lidType.compare("ZF-IMAGER") == 0) {
			sensortype = LRF::LRF_emulator::sensor_type::ZF_IMAGER;
		}//custom lidar
		std::cout << "sensor type:" << sensortype << "," << lidType << std::endl;
		LRF::LRF_emulator* lidartest = new LRF::LRF_emulator(sensortype);
		lidartest->setIntersectionSearcher(isearch);
		_6dof _ext = { (*lidarconf)["rx"].get<double>()
			, (*lidarconf)["ry"].get<double>()
			, (*lidarconf)["rz"].get<double>()
			, (*lidarconf)["x"].get<double>()
			, (*lidarconf)["y"].get<double>()
			, (*lidarconf)["z"].get<double>() };
		Matrix4d extParam = _6dof2m(_ext);

		fp.lidars.push_back(lidartest);
		fp.ext_Lidar.push_back(extParam);
	}

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

	fp.setTime(motiontimes, motiondatas);
	fp.scan(scan_start, scan_end);


    std::cout << "Hello World!\n"; 
}

