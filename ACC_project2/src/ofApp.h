#pragma once

#include "ofMain.h"

#include "ofxOpenCv.h"
#include "ofxNetwork.h"

#define _USE_LIVE_VIDEO		// uncomment this to use a live camera
// otherwise, we'll use a movie file

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

	//variable to send data
	string posData;
	
	//UDP stuff
	ofxUDPManager udpConnection;

	ofTrueTypeFont  mono;
	ofTrueTypeFont  monosm;
	vector<ofPoint> stroke;

	vector<ofPoint> remoteStroke;

	vector<string> myIP;
	vector<string> remIP;

	//a function for retrieving your local IP, written by davidemania
	//https://forum.openframeworks.cc/t/getting-ip-address-of-local-machine/27404
	vector<string> ofApp::getLocalIPs();
};

