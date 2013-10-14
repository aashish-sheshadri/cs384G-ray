#include "light.h"



using namespace std;

double DirectionalLight::distanceAttenuation( const Vec3d& P ) const
{
	return 1;}


Vec3d DirectionalLight::shadowAttenuation( const Vec3d& P ) const
{
    ray rayToLight(P,getDirection(P),ray::SHADOW);
    isect i;
    //*improve* better translucent shadows
    if(scene->intersect( rayToLight, i )){
        Vec3d returnColor(0.0f,0.0f,0.0f);
        Vec3d kTransmit = i.getMaterial().kt(i);
        double currentPoint = i.t;
        double distBetween = 1.0f;
        isect internalI;
        Vec3d pointOnObject = rayToLight.at(currentPoint);
        if(scene->intersect(ray(pointOnObject,getDirection(pointOnObject),ray::SHADOW),internalI))
            distBetween = internalI.t;
        double attentuation = std::min(1.0,(double)1.0/(double)(0.2 + 0.2f*distBetween + 0.6f*distBetween*distBetween));
        if(kTransmit[0]>0||kTransmit[1]>0||kTransmit[2]>0){
            returnColor = attentuation * getColor(P);
            returnColor%=kTransmit;}
        return returnColor;}
    return Vec3d(1,1,1);}

Vec3d DirectionalLight::getColor( const Vec3d& P ) const
{
	// Color doesn't depend on P 
	return color;
}

Vec3d DirectionalLight::getDirection( const Vec3d& P ) const
{
	return -orientation;
}

double PointLight::distanceAttenuation( const Vec3d& P ) const
{
	Vec3d tempVec = position - P;
	double d = tempVec.length();
	return std::min(1.0,(double)1.0/(double)(constantTerm + linearTerm*d + quadraticTerm*d*d));}

Vec3d PointLight::getColor( const Vec3d& P ) const
{
	// Color doesn't depend on P 
	return color;
}

Vec3d PointLight::getDirection( const Vec3d& P ) const
{
	Vec3d ret = position - P;
	ret.normalize();
	return ret;
}


Vec3d PointLight::shadowAttenuation(const Vec3d& P) const
{
    Vec3d tempVec = position - P;
    double t = tempVec.length();
    tempVec.normalize();
    ray rayToLight(P,tempVec,ray::SHADOW);
    isect i;
    //*improve* better translucent shadows
    if(scene->intersect( rayToLight, i )){
        if(t>i.t){
            Vec3d returnColor(0.0f,0.0f,0.0f);
            Vec3d kTransmit = i.getMaterial().kt(i);
            double currentPoint = i.t;
            double distBetween = 1.0f;
            isect internalI;
            Vec3d pointOnObject = rayToLight.at(currentPoint);
            if(scene->intersect(ray(pointOnObject,getDirection(pointOnObject),ray::SHADOW),internalI))
                distBetween = internalI.t;
            double attentuation = std::min(1.0,(double)1.0/(double)(constantTerm + linearTerm*distBetween + quadraticTerm*distBetween*distBetween));
            if(kTransmit[0]>0||kTransmit[1]>0||kTransmit[2]>0){
                returnColor = attentuation * getColor(P);
                returnColor%=kTransmit;}
            return returnColor;}}
    return Vec3d(1,1,1);}
