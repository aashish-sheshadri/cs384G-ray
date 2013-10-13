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
        Vec3d returnColor = 0.5f*(getColor(P) + i.getMaterial().kd(i));
        returnColor%=i.getMaterial().kt(i);
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
            Vec3d returnColor = 0.5f*(getColor(P) + i.getMaterial().kd(i));
            returnColor%=i.getMaterial().kt(i);
                return returnColor;}}
    return Vec3d(1,1,1);}
