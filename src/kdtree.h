#ifndef KDTREE_H
#define KDTREE_H
#include "scene/bbox.h"
#include <vector>
#include"scene/scene.h"
#include <algorithm>
#include <utility>
#include <cassert>
#include <unordered_set>

template<typename T>
struct ComparePair{
bool operator()(T obj1, T obj2){
    if(obj1.first<obj2.first)
        return true;
    else
        return false;}};

template<typename T>
class Node{
public:
    typedef T object_data_type;
    typedef typename std::vector<object_data_type>::iterator iterator;
    typedef Node<T>* node_pointer;
private:
    node_pointer _positiveHalf;
    node_pointer _negetiveHalf;
    Vec3d _splittingPlaneNormal;
    double _splittingPlaneDist;
    BoundingBox _box;
    std::vector<object_data_type> _objects;
public:
    Node():_positiveHalf(NULL),
        _negetiveHalf(NULL),
        _splittingPlaneNormal(Vec3d(0.0f,0.0f,0.0f)),
        _splittingPlaneDist(0.0f),
        _box(BoundingBox()){}

    ~Node(){
        if(_positiveHalf!=NULL)
            delete _positiveHalf;
        if(_negetiveHalf!=NULL)
            delete _negetiveHalf;}

    void addObject(const object_data_type& obj){
        _objects.push_back(obj);}

    double getObjectArea(){

    }

    double getBoxArea(){

    }

    iterator getBeginIterator(){
        return _objects.begin();}

    iterator getEndIterator(){
        return _objects.end();}

    bool hasObjects(){
        return !_objects.empty();}

    unsigned int getNumObjects(){
        return _objects.size();}};

template<typename T>
class KdTree
{
public:
    typedef T object_data_type;
private:
    typename Node<object_data_type>::node_pointer _root;
    bool addNode(){

    }

    double computeH(Node<object_data_type>* node, int dim){
        if(!node->hasObjects()){
            return -1.0f;}

        double totalArea = node->getBoxArea();
        double min = 1.0e308; // 1.0e308 is close to infinity... close enough for us!

        std::vector<std::pair<double, typename Node<object_data_type>::iterator> > objDistancePairs;
        objDistancePairs.reserve(2*node->getNumObjects());

        for(typename Node<object_data_type>::iterator it = node->getBeginIterator(); it!=node->getEndIterator(); ++it){
            assert(!((*it).isEmpty()));
            double d1;
            double d2;
            Vec3d normal(0.0f,0.0f,0.0f);
            (*it).getPlaneNormsDists(dim, normal, d1, d2);
            objDistancePairs.push_back(std::make_pair(d1,it));
            objDistancePairs.push_back(std::make_pair(d2,it));}
        typedef std::pair<double, typename Node<object_data_type>::iterator > internalType;
        std::cout<<std::endl;
        std::sort(objDistancePairs.begin(), objDistancePairs.end(), ComparePair<internalType>());
        std::unordered_set<typename Node<object_data_type>::iterator> transientSet;
        return 0.0f;}

public:
    KdTree():_root(NULL){}
    bool buildTree(std::vector<Geometry*>::const_iterator beginObjectsIt, std::vector<Geometry*>::const_iterator endObjectsIt){
        if(_root!=NULL)
            return false;
        _root = new Node<object_data_type>();
        while(beginObjectsIt!=endObjectsIt){
            _root->addObject((*beginObjectsIt)->getBoundingBox());
            ++beginObjectsIt;}
        computeH(_root,0);}

    void deleteTree(){
        if(_root == NULL)
            return;
        delete _root;}};



#endif // KDTREE_H
