/*
*  ContourFinder.cpp
*  
*
*  Created on 2/2/09.
*  Adapted from openframeworks ofxCvContourFinder
*
*/

#include "ContourFinder.h"

//--------------------------------------------------------------------------------
ContourFinder::ContourFinder(){
	myMoments = (CvMoments*)malloc( sizeof(CvMoments) );
	reset();
}

//--------------------------------------------------------------------------------
ContourFinder::~ContourFinder(){
	free( myMoments );
}

//--------------------------------------------------------------------------------
void ContourFinder::reset() {
    blobs.clear();
    nBlobs = 0;

	fingers.clear();
	nFingers = 0;
	
	objects.clear();
	nObjects = 0;
}

//--------------------------------------------------------------------------------
void ContourFinder::setTemplateUtils(TemplateUtils* _templates){
	templates=_templates;
}

//--------------------------------------------------------------------------------
int ContourFinder::findContours(	ofxCvGrayscaleImage&  input,
									int minArea,
									int maxArea,
									int nConsidered,
									double hullPress,	
									bool bFindHoles,
									bool bUseApproximation  ) {
	reset();

	// opencv will clober the image it detects contours on, so we want to
    // copy it into a copy before we detect contours.  That copy is allocated
    // if necessary (necessary = (a) not allocated or (b) wrong size)
	// so be careful if you pass in different sized images to "findContours"
	// there is a performance penalty, but we think there is not a memory leak
    // to worry about better to create mutiple contour finders for different
    // sizes, ie, if you are finding contours in a 640x480 image but also a
    // 320x240 image better to make two ContourFinder objects then to use
    // one, because you will get penalized less.

	if( inputCopy.width == 0 ) {
		inputCopy.allocate( input.width, input.height );
		inputCopy = input;
	} else {
		if( inputCopy.width == input.width && inputCopy.height == input.height ) 
			inputCopy = input;
		else {
			// we are allocated, but to the wrong size --
			// been checked for memory leaks, but a warning:
			// be careful if you call this function with alot of different
			// sized "input" images!, it does allocation every time
			// a new size is passed in....
			//inputCopy.clear();
			inputCopy.allocate( input.width, input.height );
			inputCopy = input;
		}
	}

	CvSeq* contour_list = NULL;
	contour_storage = cvCreateMemStorage( 1000 );
	storage	= cvCreateMemStorage( 1000 );

	int  retrieve_mode
        = (bFindHoles) ? CV_RETR_LIST : CV_RETR_EXTERNAL;
	cvFindContours( inputCopy.getCvImage(), contour_storage, &contour_list,
                    sizeof(CvContour), retrieve_mode, bUseApproximation ? CV_CHAIN_APPROX_SIMPLE : CV_CHAIN_APPROX_NONE );
	
	CvSeq* contour_ptr = contour_list;

	nCvSeqsFound = 0;

	// put the contours from the linked list, into an array for sorting
	while( (contour_ptr != NULL) )  {
		CvBox2D box = cvMinAreaRect2(contour_ptr);
		int objectId; // If the contour is an object, then objectId is its ID
		objectId = (bTrackObjects)? templates->getTemplateId(box.size.width,box.size.height): -1;
		
		if(objectId != -1 ) { //If the blob is a object
			Blob blob		= Blob();
			blob.id			= objectId;
			blob.isObject	= true;
			float area = cvContourArea( contour_ptr, CV_WHOLE_SEQ );

			cvMoments( contour_ptr, myMoments );
		
			// this is if using non-angle bounding box
			CvRect rect	= cvBoundingRect( contour_ptr, 0 );
			blob.boundingRect.x      = rect.x;
			blob.boundingRect.y      = rect.y;
			blob.boundingRect.width  = rect.width;
			blob.boundingRect.height = rect.height;

			//For anglebounding rectangle
			blob.angleBoundingBox=box;
			blob.angleBoundingRect.x	  = box.center.x;
			blob.angleBoundingRect.y	  = box.center.y;
			blob.angleBoundingRect.width  = box.size.height;
			blob.angleBoundingRect.height = box.size.width;
			blob.angle = box.angle;

			//TEMPORARY INITIALIZATION TO 0, Will be calculating afterwards.This is to prevent sending wrong data
			blob.D.x = 0;
			blob.D.y = 0;
			blob.maccel = 0;

			// assign other parameters
			blob.area                = fabs(area);
			blob.hole                = area < 0 ? true : false;
			blob.length 			 = cvArcLength(contour_ptr);
		
			blob.centroid.x			 = (myMoments->m10 / myMoments->m00);
			blob.centroid.y 		 = (myMoments->m01 / myMoments->m00);
			blob.lastCentroid.x 	 = 0;
			blob.lastCentroid.y 	 = 0;

			// get the points for the blob:
			CvPoint           pt;
			CvSeqReader       reader;
			cvStartReadSeq( contour_ptr, &reader, 0 );
	
    		for( int j=0; j < contour_ptr->total; j++ ) {
				CV_READ_SEQ_ELEM( pt, reader );
				blob.pts.push_back( ofPoint((float)pt.x, (float)pt.y) );
			}
			blob.nPts = blob.pts.size();

			objects.push_back(blob);
			
		} else if ( bTrackBlobs ) { // SEARCH FOR BLOBS
			float area = fabs( cvContourArea(contour_ptr, CV_WHOLE_SEQ) );
			if( (area > minArea) && (area < maxArea) ) {
				Blob blob=Blob();
				float area = cvContourArea( contour_ptr, CV_WHOLE_SEQ );
				cvMoments( contour_ptr, myMoments );
				
				// this is if using non-angle bounding box
				CvRect rect	= cvBoundingRect( contour_ptr, 0 );
				blob.boundingRect.x      = rect.x;
				blob.boundingRect.y      = rect.y;
				blob.boundingRect.width  = rect.width;
				blob.boundingRect.height = rect.height;
				
				//Angle Bounding rectangle
				blob.angleBoundingRect.x	  = box.center.x;
				blob.angleBoundingRect.y	  = box.center.y;
				blob.angleBoundingRect.width  = box.size.height;
				blob.angleBoundingRect.height = box.size.width;
				blob.angle = box.angle;
				
				// assign other parameters
				blob.area                = fabs(area);
				blob.hole                = area < 0 ? true : false;
				blob.length 			 = cvArcLength(contour_ptr);
				// AlexP
				// The cast to int causes errors in tracking since centroids are calculated in
				// floats and they migh land between integer pixel values (which is what we really want)
				// This not only makes tracking more accurate but also more fluid
				blob.centroid.x			 = (myMoments->m10 / myMoments->m00);
				blob.centroid.y 		 = (myMoments->m01 / myMoments->m00);
				blob.lastCentroid.x 	 = 0;
				blob.lastCentroid.y 	 = 0;
				
				// get the points for the blob:
				CvPoint           pt;
				CvSeqReader       reader;
				cvStartReadSeq( contour_ptr, &reader, 0 );
				
    			for( int j=0; j < min(TOUCH_MAX_CONTOUR_LENGTH, contour_ptr->total); j++ ) {
					CV_READ_SEQ_ELEM( pt, reader );
					blob.pts.push_back( ofPoint((float)pt.x, (float)pt.y) );
				}
				blob.nPts = blob.pts.size();
				
				blobs.push_back(blob);
			}
		} 
		contour_ptr = contour_ptr->h_next;
	}
		
	if ( bTrackFingers ) {  // SEARCH FOR FINGERS
		CvPoint*		PointArray;
		int*			hull;
		int				hullsize;
		
		if (contour_list)
			contour_list = cvApproxPoly(contour_list, sizeof(CvContour), storage, CV_POLY_APPROX_DP, hullPress, 1 );
			
		for( ; contour_list != 0; contour_list = contour_list->h_next ){
			int count = contour_list->total; // This is number point in contour
				
			CvRect rect = cvBoundingRect(contour_list, 1);
			
			if ( (rect.width*rect.height) > 300 ){		// Analize the bigger contour
				CvPoint center;
				center.x = rect.x+rect.width/2;
				center.y = rect.y+rect.height/2;
				
				PointArray = (CvPoint*)malloc( count*sizeof(CvPoint) ); // Alloc memory for contour point set.
				hull = (int*)malloc(sizeof(int)*count);	// Alloc memory for indices of convex hull vertices.
					
				cvCvtSeqToArray(contour_list, PointArray, CV_WHOLE_SEQ); // Get contour point set.
					
				// Find convex hull for curent contour.
				/*cvConvexHull(PointArray,
								count,
								NULL,
								CV_COUNTER_CLOCKWISE,
								hull,
								&hullsize); */

				CvSeq* cvch = cvConvexHull2( PointArray, 
								NULL,
								CV_COUNTER_CLOCKWISE,
								count);

				hull = (int*) cvch;
				hullsize = cvch->delta_elems;
					
				int upper = 640, lower = 0;
				for	(int j=0; j<hullsize; j++) {
					int idx = hull[j]; // corner index
					if (PointArray[idx].y < upper) 
						upper = PointArray[idx].y;
					if (PointArray[idx].y > lower) 
						lower = PointArray[idx].y;
				}
				
				float cutoff = lower - (lower - upper) * 0.1f;
				// find interior angles of hull corners
				for (int j=0; j<hullsize; j++) {
					int idx = hull[j]; // corner index
					int pdx = idx == 0 ? count - 1 : idx - 1; //  predecessor of idx
					int sdx = idx == count - 1 ? 0 : idx + 1; // successor of idx
						
					cv::Point v1 = cv::Point(PointArray[sdx].x - PointArray[idx].x, PointArray[sdx].y - PointArray[idx].y);
					cv::Point v2 = cv::Point(PointArray[pdx].x - PointArray[idx].x, PointArray[pdx].y - PointArray[idx].y);
						
					float angle = acos( (v1.x*v2.x + v1.y*v2.y) / (norm(v1) * norm(v2)) );
						
					// low interior angle + within upper 90% of region -> we got a finger
					if (angle < 1 ){ //&& PointArray[idx].y < cutoff) {
						Blob blob = Blob();
						
						//float area = cvContourArea( contour_ptr, CV_WHOLE_SEQ );
						//cvMoments( contour_ptr, myMoments );
						
						// this is if using non-angle bounding box
						//CvRect rect	= cvBoundingRect( contour_ptr, 0 );
						blob.boundingRect.x      = PointArray[idx].x-5;
						blob.boundingRect.y      = PointArray[idx].y-5;
						blob.boundingRect.width  = 10;
						blob.boundingRect.height = 10;
						
						//Angle Bounding rectangle
						blob.angleBoundingRect.x	  = PointArray[idx].x-5;
						blob.angleBoundingRect.y	  = PointArray[idx].y-5;
						blob.angleBoundingRect.width  = 10;
						blob.angleBoundingRect.height = 10;
						blob.angle = atan2(PointArray[idx].x - center.x , PointArray[idx].y - center.y);
						
						// assign other parameters
						//blob.area                = fabs(area);
						//blob.hole                = area < 0 ? true : false;
						//blob.length 			 = cvArcLength(contour_ptr);
						// AlexP
						// The cast to int causes errors in tracking since centroids are calculated in
						// floats and they migh land between integer pixel values (which is what we really want)
						// This not only makes tracking more accurate but also more fluid
						blob.centroid.x			 = PointArray[idx].x;//(myMoments->m10 / myMoments->m00);
						blob.centroid.y 		 = PointArray[idx].y;//(myMoments->m01 / myMoments->m00);
						blob.lastCentroid.x 	 = 0;
						blob.lastCentroid.y 	 = 0;
						
						fingers.push_back(blob);
					}
				}
				// Free memory.
				free(PointArray);
				free(hull);
			}
		}
	}
	
	nBlobs = blobs.size();
	nFingers = fingers.size();
	nObjects = objects.size();
	
	// Free the storage memory.
	// Warning: do this inside this function otherwise a strange memory leak
	if( contour_storage != NULL )
		cvReleaseMemStorage(&contour_storage);
	
	if( storage != NULL )
		cvReleaseMemStorage(&storage);

	return (bTrackFingers)? nFingers:nBlobs;
}