#include "ofApp.h"

#define RECONNECT_TIME 400

//ACC Project 2
//Group members: Alex Nathanson & Kevin Li

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetFrameRate(60);

	setupGui();

	urColor.r = 0;
	urColor.g = 255;
	urColor.b = 0;

	remColor.r = 255;
	remColor.g = 0;
	remColor.b = 0;
	
	vidGrabber.setVerbose(true);
	vidGrabber.setDeviceID(0);
	//were running open CV on this smaller image, but scaling up the data
	vidGrabber.initGrabber(320, 240);
	//vidGrabber.videoSettings();

	//must allocate pixels before reading anything in to them
	colorImg.allocate(320, 240);
	grayImage.allocate(320, 240);
	grayBg.allocate(320, 240);
	grayDiff.allocate(320, 240);

	threshold = 20;

	//makes it so it doesn't clear the background automatically
	ofSetBackgroundAuto(false);

	//------UDP Stuff----------------------------------------------------------------------------------------
	
	myIP = getLocalIPs();
	getSplat(myIP[0]);
	idLen = myIP[0].length();

	const char *charSplat = subnetSplat.c_str();


	//make this a global variable
	ipPort = 11999;

	//create the socket and set to send to 127.0.0.1:11999
	udpConnection.Create();
	//127.0.0.1 for loop back
	udpConnection.Bind(ipPort);
	//if the subnet splat method isn't working as intended manually enter your partner's IP
	//udpConnection.Connect(charSplat, ipPort);
	udpConnection.Connect("172.16.14.164", ipPort);
	udpConnection.SetNonBlocking(true);

	incomingIP = "";
	firstConnection = false;
}

void ofApp::setupGui() {

	gaussianBlur.addListener(this, &ofApp::gaussianBlurChanged);
	blur.addListener(this, &ofApp::blurChanged);

	gui.setup();
	gui.add(screenSize.setup("screen size", ofToString(ofGetWidth()) + "x" + ofToString(ofGetHeight())));
	gui.add(flipHorizontal.setup("flip horizontally", false));
	gui.add(flipVertical.setup("flip vertically", false));
	gui.add(invert.setup("invert", false));
	gui.add(gaussianBlur.setup("gaussian blur", 1, 1, 20));
	gui.add(blur.setup("blur", 1, 1, 20));
	gui.add(thresholdValue.setup("threshold value", 100, 1, 255));
	gui.add(dilateMultiple.setup("dilation multiple", 0, 0, 10));
	gui.add(erodeMultiple.setup("erosion multiple", 0, 0, 10));
	gui.add(polylineSmoothShape.setup("polyline smooth shape", 0.0, 0.0, 1.0));
	gui.add(polylineSmoothSize.setup("polyline smooth size", 0, 0, 32));
	gui.add(minContour.setup("minimum contour size", 10000, 0.0, (640 * 360) / 3));

}

//--------------------------------------------------------------
void ofApp::update() {
	ofBackground(200,200,200);


	//-------UDP stuff----------------------------
	char udpMessage[100000];
	udpConnection.Receive(udpMessage, 100000);

	//remIP is a vector so we could expand this for more than 2 networked users
	remIP.clear();
	remIP.push_back(" ");
	success = udpConnection.GetRemoteAddr(remIP[0], ipPort);

	inMessage = udpMessage;
	//std::cout << myIP[0] << " " << inMessage.substr(0, idLen) << "\n";

	confirmContact(inMessage);

	storeMessage(inMessage);

	//---- openCV stuff ------------------------------------------------------
	bool bNewFrame = false;

	#ifdef _USE_LIVE_VIDEO
		vidGrabber.update();
		bNewFrame = vidGrabber.isFrameNew();
	#else
		vidPlayer.update();
		bNewFrame = vidPlayer.isFrameNew();
	#endif

	if (bNewFrame) {
#ifdef _USE_LIVE_VIDEO
		colorImg.setFromPixels(vidGrabber.getPixels());
#else
		colorImg.setFromPixels(vidPlayer.getPixels());
#endif

		if (flipHorizontal && flipVertical) {
			colorImg.mirror(1, 1);
		}
		else if (flipHorizontal) {
			colorImg.mirror(0, 1);
		}
		else if (flipVertical) {
			colorImg.mirror(1, 0);
		}

		if (invert) {
			colorImg.invert();
		}

		grayImage = colorImg;
		if (bLearnBakground) {
			grayBg = grayImage;
			bLearnBakground = false;
		}

		grayDiff.absDiff(grayBg, grayImage);
		grayDiff.blur(blur);
		grayDiff.blurGaussian(gaussianBlur);
		grayDiff.threshold(thresholdValue);
		for (int i = 0; i < dilateMultiple; i++) {
			grayDiff.dilate();
		}
		for (int i = 0; i < erodeMultiple; i++) {
			grayDiff.erode();
		}

		contourFinder.findContours(grayDiff, minContour, (320 * 240) / 3, 10, false);

		//this vector needs to be reset everytime
		allBlobs.clear();

		if (contourFinder.nBlobs != 0) { //crucial - will get a vector out of range without it
			std::cout << "Original amount of blobs: " << contourFinder.nBlobs << "\n";
			for (int i = 0; i < contourFinder.nBlobs; i++) { //add this in to create a vector containing points from all blobs
				scaleData.clear();
				posData.clear();
				for (int t = 0; t < contourFinder.blobs[i].nPts; t++) {
					//posData.push_back(contourFinder.blobs[i].pts[t]);
					scaleData.push_back(contourFinder.blobs[i].pts[t]);
					scaleData[t][0] = ofMap(scaleData[t][0], 0.0, 320.0, 0.0, 1024.0, true);
					scaleData[t][1] = ofMap(scaleData[t][1], 0.0, 240.0, 0.0, 768.0, true);
				}
				allBlobs.push_back(scaleData);
			}
		}

		sendPoints(allBlobs);
	}

	//vidGrabber.update();

	//if (vidGrabber.isFrameNew()) {

	//	colorImg.setFromPixels(vidGrabber.getPixels());
	//	grayImage = colorImg;

	//	//if (bLearnBakground == true) {
	//	//	grayBg = grayImage;		// the = sign copys the pixels from grayImage into grayBg (operator overloading)
	//	//	bLearnBakground = false;
	//	//}

	//	// take the abs value of the difference between background and incoming and then threshold:
	//	grayDiff.absDiff(grayBg, grayImage);
	//	grayBg = grayImage;
	//	grayDiff.threshold(threshold);

	//	// find contours which are between the size of 20 pixels and 1/3 the w*h pixels.
	//	// also, find holes is set to true so we will get interior contours as well....
	//	contourFinder.findContours(grayDiff, 500, (340 * 240) / 3, 4, true);	// find holes

	//	//this vector needs to be reset everytime
	//	allBlobs.clear();

	//	if (contourFinder.nBlobs != 0) { //crucial - will get a vector out of range without it
	//		std::cout << "Original amount of blobs: " << contourFinder.nBlobs << "\n";
	//		for (int i = 0; i < contourFinder.nBlobs; i++) { //add this in to create a vector containing points from all blobs
	//			scaleData.clear();
	//			posData.clear();
	//			for (int t = 0; t < contourFinder.blobs[i].nPts; t++) {
	//				//posData.push_back(contourFinder.blobs[i].pts[t]);
	//				scaleData.push_back(contourFinder.blobs[i].pts[t]);
	//				scaleData[t][0] = ofMap(scaleData[t][0], 0.0, 320.0, 0.0, 1024.0, true);
	//				scaleData[t][1] = ofMap(scaleData[t][1], 0.0, 240.0, 0.0, 768.0, true);
	//			}
	//			allBlobs.push_back(scaleData);
	//		}
	//	}


	//	sendPoints(allBlobs);
	//	//testPoints(posData);
	//}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackgroundGradient(ofColor::white, ofColor::gray);

	// draw the original local video image
	ofSetHexColor(0xffffff);
	ofPushMatrix();
	//ofTranslate(ofGetWidth() / 2 - colorImg.getWidth() / 2, ofGetHeight() / 2 - colorImg.getHeight() / 2);
	if (bShowVideo) {
		colorImg.resize(ofGetWidth(), ofGetHeight());
		colorImg.draw(0, 0);
		grayDiff.draw(0, ofGetHeight() - grayDiff.getWidth());
	}
	ofPopMatrix();

	// ------text--------------
	ofPushStyle();
	ofFill();
	ofSetColor(ofColor::fuchsia);
	ofDrawRectangle(0, 0, 375, 100);
	ofSetColor(urColor);
	ofDrawRectangle(0, 85, 375, 20);
	ofSetHexColor(0x101010);
	ofDrawBitmapString("Remote CV", 10, 20);
	ofDrawBitmapString("Your local IP address: " + myIP[0], 10, 40);
	ofDrawBitmapString("Your partner's IP address: " + incomingIP, 10, 60);
	ofDrawBitmapString("Your subnet splat address: " + subnetSplat, 10, 80);
	ofDrawBitmapString("Your color is: " + ofToString(urColor), 10, 100);

	ofSetColor(ofColor::fuchsia);
	ofDrawBitmapString("Press 'v' to toggle video and path drawing", ofGetWidth() / 2.0, ofGetHeight() - 100.0);
	ofPopStyle();

	//--------UDP Stuff------------------------------------
	ofPushStyle();

	//changed from localData to scaleData, because the UDP wasn't looping back properly
	ofSetColor(urColor);
	drawPoints(allBlobs);

	ofSetColor(remColor);
	drawPoints(allRem);

	ofPopStyle();
	
	// Draw GUI
	if (!bHide) {
		gui.draw();
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

	switch (key) {
	case 'v':
		ofBackground(ofColor::white);
		bShowVideo = !bShowVideo;
		break;
	case 'h':
		bHide = !bHide; // Toggle GUI
		break;
	case 's':
		gui.saveToFile("settings.xml"); // Save GUI Settings
		break;
	case 'l':
		gui.loadFromFile("settings.xml"); // Load GUI Settings
		break;
	case ' ':
		bLearnBakground = true; // Re-save background for difference.
		break;
	}
}


void ofApp::gaussianBlurChanged(int &change) {
	if (change % 2 == 0) gaussianBlur = change + 1;
	else gaussianBlur = change;
}

void ofApp::blurChanged(int &change) {
	if (change % 2 == 0) blur = change + 1;
	else blur = change;
}


//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
	screenSize = ofToString(w) + "x" + ofToString(h);
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
}

//this method gets your local IP via the command line. Could also be accomplished with a similar method as the one used to get the remote IPs.
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

//get the subnet's splat address
void ofApp::getSplat(string locIP) {
	
#ifdef TARGET_WIN32

	string commandResult = ofSystem("arp -a");
	//ofLogVerbose() << commandResult;

	//print arp -a results
	//std::cout << commandResult << "\n";

	//create a vector containing each line of the results
	vector<string> splitSorted = ofSplitString(commandResult, "\n");

	//split your local IP into subnets sections
	vector<string> locVector;
	locVector = ofSplitString(locIP, ".");

	// the first line is actually a blank line and the next 2 lines dont contain what we need so we'll start at 3
	for (int i = 3; i < splitSorted.size(); i++) {
		//remove the two spaces in the beginning of the line
		splitSorted[i] = splitSorted[i].erase(0, 2);

		//remove the non IP elements from each line		
		int space = splitSorted[i].find_first_of(" ");
		splitSorted[i] = splitSorted[i].substr(0, space);

		//print just IPs
		//std:cout << splitSorted[i] << "\n";

		vector<string> subNet;
		subNet = ofSplitString(splitSorted[i], ".");

		if (subNet[0] == locVector[0] && subNet[1] == locVector[1] && subNet[3] == "255") {
			subnetSplat = splitSorted[i];
			//print the subnet splat ip
			//std::cout << subnetSplat << "\n";
			break;
		}
	}

	/*
#else
	
	//code for non-windows systems goes here

	*/
#endif
}

void ofApp::confirmContact(string inMess) {
	//check if the message exits and confirm it's not coming from your IP
	if (success == true && inMess.substr(0, idLen) != myIP[0])
	{
		incomingIP = ofToString(remIP[0]);
		//std::cout << "Ip Address: " << incomingIP << "\n";
		firstConnection = true;
	}
	else
	{
		// failure
		if (firstConnection == false) {
			incomingIP = "No connection";
		}
	}
}

void ofApp::storeMessage(string inMess) {
	//store incoming messages
	//check that the mesage isn't empty and it's not coming from your IP
	//if (inMessage != "" && inMessage.substr(0, idLen) != myIP[0]) {

	if (inMess != "") {

		//would be rad to generate a unique color (hash method? from their IP?
		
		if (inMess.substr(0, idLen) == myIP[0]) {
			/*
			//strip the IP off
			//inMess = inMess.erase(0, idLen);
			//std::cout << "test" << "\n";
			localPos.clear();

			float x, y;
			vector<string> strPoints = ofSplitString(inMess, "[/p]");
			for (unsigned int i = 0; i < strPoints.size(); i++) {
				vector<string> point = ofSplitString(strPoints[i], "|");
				if (point.size() == 2) {
					x = atof(point[0].c_str());
					y = atof(point[1].c_str());
					localPos.push_back(ofPoint(x, y));
				}
			}*/
		} else {
			//strip the IP off - not necessary becasue the if statement disregards splits larger than 2
			//inMess = inMess.erase(0, idLen);

			remotePos.clear();

			float x, y;

			blobVect.clear();

			blobVect = ofSplitString(inMess, "[$]");
			
			allRem.clear();
			strPoints.clear();

			if (blobVect.size() != 0) {
				for (int v = 0; v < blobVect.size(); v++) {
					strPoints = ofSplitString(blobVect[v], "[/p]");
					for (unsigned int i = 0; i < strPoints[v].size(); i++) {
						vector<string> point = ofSplitString(strPoints[v], "|");
						if (point.size() == 2) {
							x = atof(point[0].c_str());
							y = atof(point[1].c_str());
							remotePos.push_back(ofPoint(x, y));
						}
						allRem.push_back(remotePos);
					}
				}
			}
		}
		
	}
}

void ofApp::sendPoints(vector<vector <ofPoint>> points) {
	
	outMessage = "";

	for (int i = 0; i < points.size(); i++) {
		std::cout<<"Sending blobs: "<< points.size() << "\n";
		for (int b = 0; b < points[i].size(); b++) {
			outMessage += ofToString(points[i][b].x) + "|" + ofToString(points[i][b].y) + "[/p]"; // package this to send better
		}
		outMessage += "[$]"; //delineates end of a blob
	}

	//put your IP address in as a filter header
	outMessage = myIP[0] + outMessage;
	udpConnection.Send(outMessage.c_str(), outMessage.length());
}


void ofApp::testPoints(vector<ofPoint> points) {

	outMessage = "";

	for (int i = 0; i < points.size(); i++) {
		outMessage += ofToString(points[i].x) + "|" + ofToString(points[i].y) + "[/p]"; // package this to send better
	}

	//put your IP address in as a filter header
	string testIP = "255.255.5.255";
	outMessage = testIP + outMessage;
	udpConnection.Send(outMessage.c_str(), outMessage.length());
}

void ofApp::drawPoints(vector<vector <ofPoint> > points) {
	for (int i = 0; i<points.size(); i++) {
		std::cout << "Draw these blobs: " + points.size() << "\n";
		for (int b = 0; b < points[i].size(); b++) {
			ofDrawEllipse(points[i][b], 5, 5);
		}
	}
}