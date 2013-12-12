// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>

extern TraceUI* traceUI;

#include <iostream>
#include <fstream>

using namespace std;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through normalized window coordinates (x,y)
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.
Vec3d RayTracer::trace( double x, double y )
{
    Vec3d ret(0.0f,0.0f,0.0f);
    if(traceUI->depthOfField()){
        double newPlaneT = traceUI->getDistanceOfFocusPlane();//5;
        double aperture = traceUI->getLensDiameter();//2;
        double scale = std::max(1.0f/(double)buffer_width,1.0f/(double)buffer_height);
        int numSamples = traceUI->getSampleSizeDOF();//20;


        // Clear out the ray cache in the scene for debugging purposes,
        scene->intersectCache.clear();

        ray r( Vec3d(0,0,0), Vec3d(0,0,0), ray::VISIBILITY );

        scene->getCamera().rayThrough( x,y,r );
        Vec3d cameraPos = scene->getCamera().getEye();
        Vec3d cameraU = scene->getCamera().getU();
        Vec3d cameraV = scene->getCamera().getV();
        Vec3d cameraLook = scene->getCamera().getLook();
        cameraLook.normalize();
        double planeDistance = -newPlaneT; //may have to invert
        double normalProj = (cameraLook * r.getDirection()); //dot product
        double intersectionWt = 0;
        intersectionWt = - (cameraLook*r.getPosition() + planeDistance)/normalProj;
        Vec3d pointOnPlane = r.at(intersectionWt);

        JitterVal<double> jitter;
        std::vector<double> jitterX(numSamples,aperture * scale);
        std::vector<double> jitterY(numSamples,aperture * scale);
        std::transform(jitterX.begin(),jitterX.end(),jitterX.begin(),jitter);
        std::transform(jitterY.begin(),jitterY.end(),jitterY.begin(),jitter);
        std::vector<double>::iterator jitXIt = jitterX.begin();
        std::vector<double>::iterator jitYIt = jitterY.begin();
        for(int i=0;i<numSamples;++i){
            Vec3d newPoint = cameraPos + cameraU * *jitXIt + cameraV * *jitYIt;
            Vec3d newDir = pointOnPlane - newPoint;
            newDir.normalize();
            r = ray( newPoint, newDir, ray::VISIBILITY );
            Vec3d tempRet = traceRay( r, Vec3d(1.0,1.0,1.0), traceUI->getDepth() );
            ret = ret + tempRet;
            ++jitXIt;
            ++jitYIt;}

        ret = ret/(double)numSamples;
    } else {
        // Clear out the ray cache in the scene for debugging purposes,
        scene->intersectCache.clear();
        ray r( Vec3d(0,0,0), Vec3d(0,0,0), ray::VISIBILITY );
        scene->getCamera().rayThrough( x,y,r );
        ret = traceRay( r, Vec3d(1.0,1.0,1.0), traceUI->getDepth() );
    }
	ret.clamp();
	return ret;
}

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
Vec3d RayTracer::traceRay( const ray& r, const Vec3d& thresh, int depth )
{
    //*improve* Object in object not handled
    //*improve* add ray categories
    //*improve* use thresh (gain from shooting additional rays)
    bool intersectionFound = false;
    isect i;
    if (traceUI->acceleration()){
        intersectionFound = kdTree.rayTreeTraversal(i, r);
    } else{
        intersectionFound = scene->intersect( r, i );
    }

    if( intersectionFound ) {
        Vec3d pointOnObject = r.at(i.t);
            if(r.type() == ray::VISIBILITY){
                double viewAngle = (-1*r.getDirection())*i.N;
                (*_descriptorIterator).push_back(Descriptor(pointOnObject,viewAngle));}

        const Material& m = i.getMaterial();
        Vec3d intensity = m.shade(scene, r, i);
        if(depth < 1)
            return intensity;
        Vec3d reflectedDirectionCi(0.0f,0.0f,0.0f);
        Vec3d reflectedDirectionSi(0.0f,0.0f,0.0f);
        Vec3d reflectedDir(0.0f,0.0f,0.0f);
        updateReflectionParams(r,i,reflectedDirectionCi,reflectedDirectionSi,reflectedDir);

        Vec3d refractedDirectionSt(0.0f,0.0f,0.0f);
        Vec3d refractedDirectionCt(0.0f,0.0f,0.0f);
        Vec3d refractedDir(0.0f,0.0f,0.0f);
        bool shootRefractedRay = updateRefractionParams(r,i,m,reflectedDirectionSi,refractedDirectionSt,refractedDirectionCt, refractedDir);

        ray reflectedRay(pointOnObject, reflectedDir, ray::REFLECTION);
        ray refractedRay(pointOnObject, refractedDir, ray::REFRACTION);

        Vec3d reflectionIntensity = m.kr(i);
        Vec3d refractionIntensity(0.0f,0.0f,0.0f);

        reflectionIntensity %=  traceRay(reflectedRay, thresh, depth - 1);
        if(shootRefractedRay){
            refractionIntensity = m.kt(i);
            refractionIntensity %= traceRay(refractedRay, thresh, depth - 1);}

        intensity += reflectionIntensity;
        intensity += refractionIntensity;
        return intensity;
	} else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.
        if(traceUI->nonRealism()&&traceUI->edgeRedraw()){
            if(r.type() == ray::VISIBILITY){
                (*_descriptorIterator).push_back(Descriptor(Vec3d(0.0f,0.0f,0.0f),2.0f));}}
        return Vec3d( 0.5f, 0.5f, 0.5f );
	}
}

void RayTracer::updateReflectionParams(const ray& r, const isect& i,Vec3d& reflectedDirectionCi, Vec3d& reflectedDirectionSi, Vec3d& reflectedDir){
    reflectedDirectionCi = (((-1)*r.getDirection())*i.N)*i.N;
    reflectedDirectionSi = reflectedDirectionCi + r.getDirection();
    reflectedDir = reflectedDirectionCi+reflectedDirectionSi;
    reflectedDir.normalize();}

bool RayTracer::updateRefractionParams(const ray& r, const isect& i, const Material& m, const Vec3d& reflectedDirectionSi, Vec3d& refractedDirectionSt, Vec3d& refractedDirectionCt, Vec3d& refractedDir){
    if((m.kt(i)[0] <= 0.0f&&m.kt(i)[1]<=0.0f&&m.kt(i)[2]<=0.0f) || checkTotalInternal(r,i))
        return false;

    if(i.N * r.getDirection() < 0.0f){
        double ratioIndex = i.getMaterial().index(i);
        ratioIndex = 1.0f/(double)ratioIndex;
        refractedDirectionSt = ratioIndex * reflectedDirectionSi;
        double temp = refractedDirectionSt*refractedDirectionSt;
        if(temp>1)
            temp = 0;
        refractedDirectionCt = (-1.0f * i.N) * std::sqrt((1.0f - temp));
    } else if(i.N * r.getDirection() > 0.0f) {
        double ratioIndex = i.getMaterial().index(i);
        refractedDirectionSt = ratioIndex * reflectedDirectionSi;
        double temp = refractedDirectionSt*refractedDirectionSt;
        if(temp>1)
            temp = 0;
        refractedDirectionCt = i.N * std::sqrt((1.0f - temp));}

    refractedDir = refractedDirectionSt + refractedDirectionCt;
    refractedDir.normalize();
    return true;}

bool RayTracer::checkTotalInternal(const ray &r, const isect &i){
    if(i.N * r.getDirection() > 0.0f){
        double incidentAngle = i.N * r.getDirection();
        double ratioIndex = i.getMaterial().index(i);
        if((1.0f - (ratioIndex*ratioIndex*(1.0f - incidentAngle * incidentAngle)))<0.0f)
            return true;}
    return false;}

RayTracer::RayTracer()
    : scene( 0 ), buffer( 0 ), buffer_width( 256 ), buffer_height( 256 ), m_bBufferReady( false ), cachedBuffer(0)
{
}


RayTracer::~RayTracer()
{
	delete scene;
	delete [] buffer;
    if(cachedBuffer!=0)
        delete [] cachedBuffer;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene( char* fn )
{
	ifstream ifs( fn );
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}
	
	// Strip off filename, leaving only the path:
	string path( fn );
	if( path.find_last_of( "\\/" ) == string::npos )
		path = ".";
	else
		path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
    Parser parser( tokenizer, path );
	try {
		delete scene;
		scene = 0;
		scene = parser.parseScene();
        if(traceUI->acceleration()){
            // *improve* do preprocessing for bounding boxes
            if(!kdTree.buildTree(scene->beginObjects(),scene->endObjects())){
                kdTree.deleteTree();
                kdTree.buildTree(scene->beginObjects(),scene->endObjects());}}}
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	}
	catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	}
	catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}


	if( ! sceneLoaded() )
		return false;

	return true;
}

void RayTracer::traceSetup( int w, int h )
{
	if( buffer_width != w || buffer_height != h )
	{
		buffer_width = w;
		buffer_height = h;

		bufferSize = buffer_width * buffer_height * 3;
		delete [] buffer;
		buffer = new unsigned char[ bufferSize ];
        if(cachedBuffer!=0)
            delete [] cachedBuffer;
        cachedBuffer = new unsigned char[ bufferSize ];

	}
	memset( buffer, 0, w*h*3 );
    _descriptors.clear();


    std::vector< std::vector<Descriptor> > temp(w*h);
    _descriptors = temp;
    _descriptorIterator = _descriptors.begin();
	m_bBufferReady = true;
    updateCache = true;
    distThresh = -1;//0.01;
    viewAngleThresh = -1;//0.15;
    surfaceAngleThresh = -1; //0.05
}

void RayTracer::tracePixel( int i, int j )
{
    double varThresh = 0.001f;
    Vec3d col(0.0f,0.0f,0.0f);
	if( ! sceneLoaded() )
		return;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);
    int numSamples = traceUI->getSampleSize();
    numSamples = (numSamples%2)==0?numSamples+1:numSamples;
    if(traceUI->edgeRedraw()&&traceUI->nonRealism())
        (*_descriptorIterator).reserve(numSamples*numSamples);
    if(numSamples>1){
        double xMin = i - 0.5f;
        double yMin = j - 0.5f;
        double delta = 0.5f/(double)(numSamples/2);

        std::vector<double> xVec;
        std::vector<double> yVec;
        xVec.reserve(numSamples);
        yVec.reserve(numSamples);
        int temp = numSamples - 1;
        while(temp>=0){
            xVec.push_back(xMin+temp*delta);
            yVec.push_back(yMin+temp*delta);
            --temp;}

        if(traceUI->jitter()){
            Jitter<double> jitter(delta);
            std::transform(xVec.begin(), xVec.end(), xVec.begin(), jitter);
            std::transform(yVec.begin(), yVec.end(), yVec.begin(), jitter);}

        if(traceUI->getAdapativeSampling()){
            int totalSamples = numSamples*numSamples;
            std::vector<std::pair<double,double> > allSamples;
            allSamples.reserve(totalSamples);
            for (std::vector<double>::iterator itX = xVec.begin();itX!=xVec.end();++itX){
                for(std::vector<double>::iterator itY = yVec.begin();itY!=yVec.end();++itY){
                    allSamples.push_back(std::make_pair(*itX,*itY));}}

            std::vector<double> sampleIntensities;
            sampleIntensities.reserve(totalSamples);

            fillRandomIdx(allSamples.begin(),allSamples.end());

            int minSamples = 2;
            int numUsedSamples = 0;
            double runningSum = 0.0f;
            for(std::vector<std::pair<double,double> >::iterator it = allSamples.begin();it!=allSamples.end();++it){
                ++numUsedSamples;
                Vec3d tempIntensity = trace((*it).first/double(buffer_width),(*it).second/double(buffer_height));
                double avgIntensity = (tempIntensity[0] + tempIntensity[1] + tempIntensity[2])/3.0f;
                col = col + tempIntensity;
                runningSum += avgIntensity;
                sampleIntensities.push_back(avgIntensity);
                if(minSamples<0){
                    double mean = runningSum/(double)numUsedSamples;
                    std::vector<double> zeroMean(sampleIntensities);
                    ZeroMean<double> tempMeanObj(mean);
                    std::transform(zeroMean.begin(),zeroMean.end(),zeroMean.begin(),tempMeanObj);
                    double varSum = std::accumulate(zeroMean.begin(),zeroMean.end(), 0.0f);
                    varSum = sqrt(varSum/(double)(numUsedSamples-1));
                    if(varSum < varThresh)
                        break;}
                --minSamples;}
            col = col/numUsedSamples;
        } else {
            for(std::vector<double>::iterator itX = xVec.begin();itX!=xVec.end();++itX){
                for(std::vector<double>::iterator itY = yVec.begin();itY!=yVec.end();++itY){
                    col+= trace(*itX/double(buffer_width),*itY/double(buffer_height));}}
            col/=(numSamples*numSamples);}
    } else {
        col = trace(x,y);}

	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;
	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
    pixel[2] = (int)( 255.0 * col[2]);
    ++_descriptorIterator;}

void RayTracer::drawEdges(){
    int kernalWidth = 3;
    int kernalHeight = 3;

    if(updateCache){
        std::copy(buffer,buffer + (buffer_width * buffer_height * 3), cachedBuffer);
        updateCache = false;}

    double currDistThresh = traceUI->getDepthThreshold();//0.01;
    double currViewAngleThresh = traceUI->getAngleThresholdA();//0.15;
    double currSurfaceAngleThresh = traceUI->getAngleThresholdB(); //0.05
    if(currSurfaceAngleThresh!=surfaceAngleThresh || currViewAngleThresh!=viewAngleThresh || currDistThresh!=distThresh){
        surfaceAngleThresh = currSurfaceAngleThresh;
        viewAngleThresh = currViewAngleThresh;
        distThresh = currDistThresh;
        std::copy(cachedBuffer,cachedBuffer + (buffer_width * buffer_height * 3), buffer);
    } else {
        return;}

    std::vector<std::vector<Descriptor> >::iterator beginIt(_descriptors.begin());
    int strength = 0;
    for (int y=0; y<buffer_height; y++) {
        for (int x=0; x<buffer_width; x++) {
            bool bEdge = false;
            std::vector<std::vector<std::vector<Descriptor> >::iterator > neighbours;
            std::back_insert_iterator<std::vector<std::vector<std::vector<Descriptor> >::iterator > > neighboursBackIIt (neighbours);
            loadNeighbours(y, x, kernalWidth, kernalHeight, buffer_width, buffer_height, beginIt, neighboursBackIIt);
            std::vector<std::vector<Descriptor> >::iterator thisIt = beginIt + (y*buffer_width+x);
            Vec3d thisPoint(0.0f,0.0f,0.0f);
            double thisViewAngle = 0.0f;
            loadAvgVals((*thisIt).begin(),(*thisIt).end(),thisPoint,thisViewAngle);
            for(std::vector<std::vector<std::vector<Descriptor> >::iterator >::iterator it = neighbours.begin();it!=neighbours.end();++it){
                Vec3d thisNeighbourPoint(0.0f,0.0f,0.0f);
                double thisNeighbourViewAngle = 0.0f;
                loadAvgVals((*(*it)).begin(),(*(*it)).end(),thisNeighbourPoint,thisNeighbourViewAngle);
                Vec3d diffPoint = thisPoint - thisNeighbourPoint;
                double spacialDepth = diffPoint.length();
                if(spacialDepth > distThresh){
                    bEdge = true;
                    strength+=5;
                    break;}
                if(thisNeighbourViewAngle > 1 && std::fabs(thisViewAngle) <= 1){
                    bEdge = true;
                    strength+=5;}
                if(std::fabs(thisViewAngle - thisNeighbourViewAngle)>viewAngleThresh){
                    bEdge = true;
                    ++strength;}
                if(std::fabs(thisViewAngle)<surfaceAngleThresh && std::fabs(thisViewAngle)>std::fabs(thisNeighbourViewAngle)){
                    bEdge = true;
                    strength+=5;}
            }
            if(bEdge){
                unsigned char *pixel = buffer + ( x + y * buffer_width ) * 3;
                pixel[0] = (int)(50/strength);
                pixel[1] = (int)(50/strength);
                pixel[2] = (int)(50/strength);
            }
        }
    }
}


