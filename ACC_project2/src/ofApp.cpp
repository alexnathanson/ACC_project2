#include "ofApp.h"

#define RECONNECT_TIME 400

//ACC Project 2
//Group members: Alex Nathanson & Kevin Li

//--------------------------------------------------------------
void ofApp::setup() {
	// we don't want to be running to fast
	ofSetVerticalSync(true);
	ofSetFrameRate(60);

	/*
	vidPlayer.load("fingers.mov");
	vidPlayer.play();
	vidPlayer.setLoopState(OF_LOOP_NORMAL); // a number of different loop types
	*/

	vidGrabber.setVerbose(true);
	vidGrabber.setDeviceID(0);
	vidGrabber.initGrabber(320, 240);
	//vidGrabber.videoSettings();

	//must allocate pixels before reading anything in to them
	colorImg.allocate(320, 240);
	grayImage.allocate(320, 240);
	grayBg.allocate(320, 240);
	grayDiff.allocate(320, 240);

	bLearnBakground = true;
	threshold = 10;

	//makes it so it doesn't clear the background automatically
	ofSetBackgroundAuto(false);

	//------UDP Stuff----------------------------------------------------------------------------------------
	

	//create the socket and set to send to 127.0.0.1:11999
	udpConnection.Create();
	//127.0.0.1
	udpConnection.Bind(11999);
	udpConnection.Connect("192.168.1.255",11999);
	udpConnection.SetNonBlocking(true);

	myIP = getLocalIPs();
	//std::cout << myIP[0] << "\n";
}

//--------------------------------------------------------------
void ofApp::update() {
	//ofBackground(100,100,100);

	//-------UDP stuff----------------------------
	char udpMessage[100000];
	udpConnection.Receive(udpMessage, 100000);

	string message = udpMessage;
	if (message != "") {
		remoteStroke.clear();
		float x, y;
		vector<string> strPoints = ofSplitString(message, "[/p]");
		for (unsigned int i = 0; i<strPoints.size(); i++) {
			vector<string> point = ofSplitString(strPoints[i], "|");
			if (point.size() == 2) {
				x = atof(point[0].c_str());
				y = atof(point[1].c_str());
				remoteStroke.push_back(ofPoint(x, y));
			}
		}
	}

	//---- openCV stuff ------------------------------------------------------
	bool bNewFrame = false;

	vidGrabber.update();

	//vidPlayer.update();
	bNewFrame = vidGrabber.isFrameNew();

	if (bNewFrame) {


		colorImg.setFromPixels(vidGrabber.getPixels());

		grayImage = colorImg;
		if (bLearnBakground == true) {
			grayBg = grayImage;		// the = sign copys the pixels from grayImage into grayBg (operator overloading)
			bLearnBakground = false;
		}

		// take the abs value of the difference between background and incoming and then threshold:
		grayDiff.absDiff(grayBg, grayImage);
		grayDiff.threshold(threshold);

		// find contours which are between the size of 20 pixels and 1/3 the w*h pixels.
		// also, find holes is set to true so we will get interior contours as well....
		contourFinder.findContours(grayDiff, 20, (340 * 240) / 3, 10, true);	// find holes
	}

}

//--------------------------------------------------------------
void ofApp::draw() {
	// ------text--------------
	ofFill();
	//ofSetHexColor(0xFFFFFF);
	ofDrawRectangle(0, 0, 200, 30);
	ofSetHexColor(0x101010);
	ofDrawBitmapString("Remote CV", 10, 20);
	ofDrawBitmapString("Your local IP address: " + myIP[0], 10, 40);
	ofDrawBitmapString("Your partner's IP address: " + myIP[0], 10, 60); //retrieving the remote IP hasn't been written yet
	ofDrawBitmapString("drag to draw", 10, 80);


	//----openCV stuff ---------------------------------------------------

	// draw the incoming video image
	ofSetHexColor(0xffffff);
	ofPushMatrix();
	ofTranslate(ofGetWidth() / 2 - colorImg.getWidth() / 2, ofGetHeight() / 2 - colorImg.getHeight() / 2);
	if (bShowVideo) {
		colorImg.draw(0, 0);
	}
	for (int i = 0; i < contourFinder.nBlobs; i++) {
		ofSetColor(ofColor::fuchsia);
		ofFill();
		ofDrawEllipse(contourFinder.blobs[i].boundingRect.getCenter(), 20, 20);
		
		//get the position data
		posData = ofToString(contourFinder.blobs[i].boundingRect.getCenter());
		//std:cout << "Pos Data: " << posData << "\n";

	}
	ofPopMatrix();

	ofDrawBitmapString("Press 'v' to toggle video and path drawing", ofGetWidth() / 2.0, ofGetHeight() - 100.0);

	//--------UDP Stuff------------------------------------
	//draw local data

	ofPushStyle();
	for (unsigned int i = 1; i<stroke.size(); i++) {
		ofDrawLine(stroke[i - 1].x, stroke[i - 1].y, stroke[i].x, stroke[i].y);
	}
	ofPopStyle();

	//draw remote data
	ofPushStyle();
	//color of remote image
	ofSetColor(0, 255, 0);
	for (unsigned int i = 1; i<remoteStroke.size(); i++) {
		ofDrawLine(remoteStroke[i - 1].x, remoteStroke[i - 1].y, remoteStroke[i].x, remoteStroke[i].y);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

	switch (key) {
	case 'v':
		ofBackground(ofColor::white);
		bShowVideo = !bShowVideo;
		break;
	case '+':
		threshold++;
		if (threshold > 255) threshold = 255;
		break;
	case '-':
		threshold--;
		if (threshold < 0) threshold = 0;
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	stroke.push_back(ofPoint(x, y));
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	string message = "";
	for (unsigned int i = 0; i<stroke.size(); i++) {
		message += ofToString(stroke[i].x) + "|" + ofToString(stroke[i].y) + "[/p]";
	}
	udpConnection.Send(message.c_str(), message.length());
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}

vector<string> ofApp::getLocalIPs()
{
	vector<string> result;

#ifdef TARGET_WIN32

	string commandResult = ofSystem("ipconfig");
	//ofLogVerbose() << commandResult;

	for (int pos = 0; pos >= 0; )
	{
		pos = commandResult.find("IPv4", pos);

		if (pos >= 0)
		{
			pos = commandResult.find(":", pos) + 2;
			int pos2 = commandResult.find("\n", pos);

			string ip = commandResult.substr(pos, pos2 - pos);

			pos = pos2;

			if (ip.substr(0, 3) != "127") // let's skip loopback addresses
			{
				result.push_back(ip);
				//ofLogVerbose() << ip;
			}
		}
	}

#else

	string commandResult = ofSystem("ifconfig");

	for (int pos = 0; pos >= 0; )
	{
		pos = commandResult.find("inet ", pos);

		if (pos >= 0)
		{
			int pos2 = commandResult.find("netmask", pos);

			string ip = commandResult.substr(pos + 5, pos2 - pos - 6);

			pos = pos2;

			if (ip.substr(0, 3) != "127") // let's skip loopback addresses
			{
				result.push_back(ip);
				//ofLogVerbose() << ip;
			}
		}
	}

#endif

	return result;
}