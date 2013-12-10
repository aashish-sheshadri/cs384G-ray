#include "ray.h"
#include "material.h"
#include "light.h"

#include "../fileio/bitmap.h"
#include "../fileio/pngimage.h"
#include "../ui/TraceUI.h"

using namespace std;
extern bool debugMode;
extern TraceUI* traceUI;

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
Vec3d Material::shade( Scene *scene, const ray& r, const isect& i ) const
{
    // YOUR CODE HERE

    // For now, this method just returns the diffuse color of the object.
    // This gives a single matte color for every distinct surface in the
    // scene, and that's it.  Simple, but enough to get you started.
    // (It's also inconsistent with the phong model...)

    // Your mission is to fill in this method with the rest of the phong
    // shading model, including the contributions of all the light sources.
    // You will need to call both distanceAttenuation() and shadowAttenuation()
    // somewhere in your code in order to compute shadows and light falloff.

    if( debugMode )
        std::cout << "Debugging Phong code..." << std::endl;
    Vec3d intensity(0.0f,0.0f,0.0f);
    Vec3d emmisiveIntensity = ke(i);
    Vec3d ambientIntensity = ka(i);
    ambientIntensity %=  scene->ambient();

	// When you're iterating through the lights,
	// you'll want to use code that looks something
	// like this:
	//
	const Vec3d intersectionPoint = r.at(i.t);

    const Vec3d blue(0.0f,0.0f,0.4f);
    const Vec3d yellow(0.4f,0.4f,0.0f);
    double alpha = 0.2f;
    double beta = 0.6f;
    bool bNonRealism = traceUI->nonRealism();
	for ( vector<Light*>::const_iterator litr = scene->beginLights(); litr != scene->endLights(); ++litr ){
            Light* pLight = *litr;
            Vec3d lightDirection = pLight->getDirection(intersectionPoint);
			Vec3d surfaceNormal = i.N;
			double aLightToNormal = surfaceNormal * lightDirection;
            Vec3d lightReflectedDirectionCi = (lightDirection*surfaceNormal)*surfaceNormal;
			Vec3d lightReflectedDirectionSi = lightReflectedDirectionCi - lightDirection;
            Vec3d lightReflectedDirection = lightReflectedDirectionCi + lightReflectedDirectionSi;
			lightReflectedDirection.normalize();
            double aRayToLight = (-1 * r.getDirection()) * lightReflectedDirection;
            Vec3d shadowLight(1.0f,1.0f,1.0f);

            if(bNonRealism){
                double coolFac = (1.0f+ (-1.0f)* aLightToNormal)/2.0f;
                double warmFac = 1.0f - coolFac;
                Vec3d diffuseIntensity = (coolFac * ( blue + kd(i) * alpha)) + (warmFac * ( yellow + kd(i) * beta));
                //Vec3d specularIntensity = ks(i) * pLight->getColor(intersectionPoint);
                shadowLight %= diffuseIntensity; //+ specularIntensity * std::pow(std::max(aRayToLight,0.0),shininess(i)));
            } else {
                if(bump(i, traceUI->getBumpScale())[0]!= 2.0f){
                    Vec3d perturbed = 2.0f*bump(i, traceUI->getBumpScale())-1.0f;
                    perturbed.normalize();
                    aLightToNormal = perturbed * lightDirection;}
                Vec3d diffuseIntensity = kd(i);
                Vec3d specularIntensity = ks(i);
                diffuseIntensity%=pLight->getColor(intersectionPoint);
                specularIntensity%=pLight->getColor(intersectionPoint);
                shadowLight = pLight->shadowAttenuation(intersectionPoint);
                shadowLight %= (diffuseIntensity * std::max(aLightToNormal, 0.0) + specularIntensity * std::pow(std::max(aRayToLight,0.0),shininess(i)));
            }
            intensity += shadowLight*pLight->distanceAttenuation(intersectionPoint);
    }
    return intensity + emmisiveIntensity + ambientIntensity;
}

TextureMap::TextureMap( string filename ) {

	int start = filename.find_last_of('.');
	int end = filename.size() - 1;
	if (start >= 0 && start < end) {
		string ext = filename.substr(start, end);
		if (!ext.compare(".png")) {
			png_cleanup(1);
			if (!png_init(filename.c_str(), width, height)) {
				double gamma = 2.2;
				int channels, rowBytes;
				unsigned char* indata = png_get_image(gamma, channels, rowBytes);
				int bufsize = rowBytes * height;
				data = new unsigned char[bufsize];
				for (int j = 0; j < height; j++)
					for (int i = 0; i < rowBytes; i += channels)
						for (int k = 0; k < channels; k++)
							*(data + k + i + j * rowBytes) = *(indata + k + i + (height - j - 1) * rowBytes);
				png_cleanup(1);
			}
		}
		else
			if (!ext.compare(".bmp")) data = readBMP(filename.c_str(), width, height);
			else data = NULL;
	} else data = NULL;
	if (data == NULL) {
		width = 0;
		height = 0;
		string error("Unable to load texture map '");
		error.append(filename);
		error.append("'.");
		throw TextureMapException(error);
	}
}

BumpMap::BumpMap( string filename ) {

    int start = filename.find_last_of('.');
    int end = filename.size() - 1;
    if (start >= 0 && start < end) {
        string ext = filename.substr(start, end);
        if (!ext.compare(".png")) {
            png_cleanup(1);
            if (!png_init(filename.c_str(), width, height)) {
                double gamma = 2.2;
                int channels, rowBytes;
                unsigned char* indata = png_get_image(gamma, channels, rowBytes);
                int bufsize = rowBytes * height;
                data = new unsigned char[bufsize];
                for (int j = 0; j < height; j++)
                    for (int i = 0; i < rowBytes; i += channels)
                        for (int k = 0; k < channels; k++)
                            *(data + k + i + j * rowBytes) = *(indata + k + i + (height - j - 1) * rowBytes);
                png_cleanup(1);
            }
        }
        else
            if (!ext.compare(".bmp")) data = readBMP(filename.c_str(), width, height);
            else data = NULL;
    } else data = NULL;
    if (data == NULL) {
        width = 0;
        height = 0;
        string error("Unable to load texture map '");
        error.append(filename);
        error.append("'.");
        throw TextureMapException(error);
    }
}

Vec3d TextureMap::getMappedValue( const Vec2d& coord ) const
{
	// YOUR CODE HERE

    // In order to add texture mapping support to the 
    // raytracer, you need to implement this function.
    // What this function should do is convert from
    // parametric space which is the unit square
    // [0, 1] x [0, 1] in 2-space to bitmap coordinates,
    // and use these to perform bilinear interpolation
    // of the values.
    //return Vec3d(1.0, 1.0, 1.0);

    float xCor = coord[0] * width,
    yCor = coord[1] * height;
    int lowXindex = (int)xCor;
    int lowYindex = (int)yCor;
    float deltaX = xCor - lowXindex, deltaY = yCor - lowYindex;
    float a = (1-deltaX)*(1-deltaY), b = (deltaX)*(1-deltaY), c = (1-deltaX)*deltaY, d = deltaX*deltaY;
    return a*getPixelAt(lowXindex,lowYindex) + b*getPixelAt(lowXindex+1,lowYindex) + c*getPixelAt(lowXindex,lowYindex+1) + d*getPixelAt(lowXindex+1,lowYindex+1);

}

Vec3d BumpMap::getDiffValue( const Vec2d& coord , const float scale) const
{
    float xCor = coord[0] * width,
    yCor = coord[1] * height;
    int x = (int)xCor, y = (int)yCor;
    int right = (int)x + 1;
    int top = (int)y + 1;
    Vec3d diffX = getPixelAt(x,y) - getPixelAt(right, y);
    double avgX = scale*(diffX[0]+diffX[1]+diffX[2])/(double)3.0f;
    Vec3d diffY = getPixelAt(x,y) - getPixelAt(x, top);
    double avgY = scale*(diffY[0]+diffY[1]+diffY[2])/(double)3.0f;
    //float deltaX = xCor - lowXindex, deltaY = yCor - lowYindex;
    Vec3d perturb(avgX, avgY, 1);
    perturb.normalize();
    return perturb;

}


Vec3d TextureMap::getPixelAt( int x, int y ) const
{
    // This keeps it from crashing if it can't load
    // the texture, but the person tries to render anyway.
    if (0 == data)
      return Vec3d(1.0, 1.0, 1.0);

    if( x >= width )
       x = width - 1;
    if( y >= height )
       y = height - 1;

    // Find the position in the big data array...
    int pos = (y * width + x) * 3;
    return Vec3d( double(data[pos]) / 255.0,
       double(data[pos+1]) / 255.0,
       double(data[pos+2]) / 255.0 );
}

Vec3d BumpMap::getPixelAt( int x, int y ) const
{
    // This keeps it from crashing if it can't load
    // the texture, but the person tries to render anyway.
    if (0 == data)
      return Vec3d(1.0, 1.0, 1.0);

    if( x >= width )
       x = width - 1;
    if( y >= height )
       y = height - 1;

    // Find the position in the big data array...
    int pos = (y * width + x) * 3;
    return Vec3d( double(data[pos]) / 255.0,
       double(data[pos+1]) / 255.0,
       double(data[pos+2]) / 255.0 );
}

Vec3d MaterialParameter::value( const isect& is ) const
{
    if( 0 != _textureMap )
        return _textureMap->getMappedValue( is.uvCoordinates );
    else
        return _value;
}

Vec3d MaterialParameter::valueTMP( const isect& is , const float scale) const
{
    if( 0 != _bumpMap )
        return _bumpMap->getDiffValue( is.uvCoordinates , scale);
    else
        return Vec3d(2.0f,2.0f,2.0f);
}

double MaterialParameter::intensityValue( const isect& is ) const
{
    if( 0 != _textureMap )
    {
        Vec3d value( _textureMap->getMappedValue( is.uvCoordinates ) );
        return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
    }
    else
        return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}

