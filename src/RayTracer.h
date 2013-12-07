#ifndef __RAYTRACER_H__
#define __RAYTRACER_H__

// The main ray tracer.

#include "scene/ray.h"
#include <vector>
#include <algorithm>
#include <numeric>
#include "kdtree.h"


template<typename T>
struct Jitter{
    T jitterMax;
    Jitter(T jitterMax){
        this->jitterMax = jitterMax;}
    T operator ()(T baseVal){
        float randVal = (float)rand()/RAND_MAX;
        int sign = randVal>0.5?1:-1;
        randVal = (float)rand()/RAND_MAX;
        randVal*=jitterMax;
        return (randVal*sign + baseVal);}};

template<typename T>
struct UFRand{
    unsigned int operator()(unsigned int val){
        float randVal = (float)rand()/RAND_MAX;
        randVal *= (double)val;
        return (unsigned int)(randVal + 0.5f);}};

template<typename RI>
void fillRandomIdx(RI itBegin, RI itEnd){
    unsigned int size = itEnd - itBegin;
    std::vector<unsigned int> swapIdxOne(size,size-1);
    std::vector<unsigned int> swapIdxTwo(size,size-1);
    std::transform(swapIdxOne.begin(), swapIdxOne.end(), swapIdxOne.begin(), UFRand<unsigned int>());
    std::transform(swapIdxTwo.begin(), swapIdxTwo.end(), swapIdxTwo.begin(), UFRand<unsigned int>());

    std::vector<unsigned int>::iterator itOne = swapIdxOne.begin();
    std::vector<unsigned int>::iterator itTwo = swapIdxTwo.begin();

    RI initTemp = itBegin;
    while(itBegin!=itEnd){
        typename RI::value_type temp;
        temp = initTemp[*itOne];
        initTemp[*itOne] = initTemp[*itTwo];
        initTemp[*itTwo] = temp;
        ++itBegin;
        ++itOne;
        ++itTwo;}}

template<typename T>
struct ZeroMean{
    T _mean;
    ZeroMean(T mean):_mean(mean){}
    T operator()(T val){
        T temp = val - _mean;
        return (temp*temp);}};

template<typename T>
struct SquareInPlace{
    T operator()(T val){
        val*=val;
        return val;}};

class Scene;
class RayTracer
{
public:
    RayTracer();
    ~RayTracer();

    Vec3d trace( double x, double y );
	Vec3d traceRay( const ray& r, const Vec3d& thresh, int depth );


	void getBuffer( unsigned char *&buf, int &w, int &h );
	double aspectRatio();
	void traceSetup( int w, int h );
	void tracePixel( int i, int j );

	bool loadScene( char* fn );

	bool sceneLoaded() { return scene != 0; }

    void setReady( bool ready )
      { m_bBufferReady = ready; }
    bool isReady() const
      { return m_bBufferReady; }

	const Scene& getScene() { return *scene; }

private:
    void updateReflectionParams(const ray&, const isect&, Vec3d&, Vec3d&, Vec3d&);
    bool updateRefractionParams(const ray&, const isect&, const Material&, const Vec3d&, Vec3d&, Vec3d&, Vec3d&);
	unsigned char *buffer;
	int buffer_width, buffer_height;
	int bufferSize;
	Scene* scene;
    bool checkTotalInternal(const ray& r,const isect& i);
    bool m_bBufferReady;
    KdTree<Geometry> kdTree;};

#endif // __RAYTRACER_H__
