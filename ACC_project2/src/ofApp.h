#pragma once

#include "ofMain.h"

#include "ofxOpenCv.h"
#include "ofxNetwork.h"
#include "ofxGui.h"

#define _USE_LIVE_VIDEO

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

#ifdef _USE_LIVE_VIDEO
	ofVideoGrabber 		vidGrabber;
#else
	ofVideoPlayer 		vidPlayer;
#endif

	ofxCvColorImage			colorImg;

	ofxCvGrayscaleImage 	grayImage;
	ofxCvGrayscaleImage 	grayBg;
	ofxCvGrayscaleImage 	grayDiff;

	ofxCvContourFinder 	contourFinder;

	int 				threshold;
	bool				bLearnBakground;
	bool				bShowVideo = true;

	// GUI / Contour Finding

	void				setupGui();
	void				gaussianBlurChanged(int & change);
	void				blurChanged(int & change);
	ofxLabel			screenSize;
	ofxToggle			flipHorizontal;
	ofxToggle			flipVertical;
	ofxToggle			invert;
	ofxIntSlider		gaussianBlur;
	ofxIntSlider		blur;
	ofxIntSlider		thresholdValue;
	ofxIntSlider		dilateMultiple;
	ofxIntSlider		erodeMultiple;
	ofxIntSlider		polylineSmoothSize;
	ofxIntSlider		minContour;
	ofxFloatSlider		polylineSmoothShape;

	ofxPanel gui;

	bool bHide;

	//variable to send data
	vector<ofPoint> posData;
	vector<ofPoint> scaleData;

	vector<vector<ofPoint> > allBlobs;

	//recieving stuff
	vector <string> strPoints;
	vector<string> blobVect;


	//UDP stuff
	ofxUDPManager udpConnection;

	ofTrueTypeFont  mono;
	ofTrueTypeFont  monosm;
	vector<ofPoint> stroke;

	vector<ofPoint> remotePos;
	vector<vector <ofPoint> > allRem;

	vector<ofPoint> localPos;

	ofColor ipColor;

	vector<string> myIP;
	vector<string> remIP;

	int ipPort;

	//a function for retrieving your local IP, written by davidemania
	// this uses the command line rather than GetListenAddr
	//https://forum.openframeworks.cc/t/getting-ip-address-of-local-machine/27404
	vector<string> ofApp::getLocalIPs();

	ofColor urColor;
	ofColor remColor;

	string incomingIP; //changed from ipAddress to be more descriptive
	bool firstConnection;

	//determines the splat address for a given subnet
	// only tested on mac - syntax might need to be adapted for other OS
	void getSplat(string locIP);
	string subnetSplat;

	int idLen;

	void confirmContact(string inMess);

	string inMessage;
	string outMessage;

	bool success;

	void storeMessage(string inMess);

	void sendPoints(vector<vector <ofPoint> > points);

	void drawPoints(vector<vector <ofPoint> > points);

};

