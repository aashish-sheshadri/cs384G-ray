#ifndef KDTREE_H
#define KDTREE_H
#include "scene/bbox.h"
#include <vector>
#include"scene/scene.h"
#include <algorithm>
#include <utility>
#include <cassert>
#include <set>

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
    node_pointer _positiveHalf;
    node_pointer _negativeHalf;
private:
    Vec3d _splittingPlaneNormal;
    double _splittingPlaneDist;
    BoundingBox _box;
    std::vector<object_data_type> _objects;
public:
    Node():_positiveHalf(NULL),
        _negativeHalf(NULL),
        _splittingPlaneNormal(Vec3d(0.0f,0.0f,0.0f)),
        _splittingPlaneDist(0.0f),
        _box(BoundingBox()){}

    ~Node(){
        if(_positiveHalf!=NULL)
            delete _positiveHalf;
        if(_negativeHalf!=NULL)
            delete _negativeHalf;}

    void setPlaneDist(double d){
        _splittingPlaneDist = d;}

    void setPlaneNormal(Vec3d n){
        _splittingPlaneNormal = n;}

    BoundingBox& getBoundingBox(){
        return _box;}

    void addObject(const object_data_type& obj){
        _objects.push_back(obj);}

    double getArea(object_data_type obj){
        obj.area();}

    double getBoxArea(){
        return _box.area();}

    double getObjectsArea(){
        double toReturn = 0.0f;
        for(iterator it = _objects.begin(); it!=_objects.end(); ++it){
            assert(!((*it).isEmpty()));
            toReturn += (*it).area();}
        return toReturn;}

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
    typedef typename Node<object_data_type>::node_pointer node_pointer;
private:
    double _ti, _tt;
    int _depth;
    int _minObjs;
    node_pointer _root;
    bool addNode(){

    }

    double computeH(Node<object_data_type>* node, int dim){
        if(!node->hasObjects()){
            return -1.0f;}
        double totalArea = node->getBoxArea();
        double min = 1.0e308; // 1.0e308 is close to infinity... close enough for us!

        double bestD = 0.0f;
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
        std::set<typename Node<object_data_type>::iterator> transientSet;
        int negCounter = 0, posCounter = node->getNumObjects();
        double negativeArea = 0.0f;
        double positiveArea = node->getObjectsArea();
        double hValue, negP, posP;
        typename std::vector<internalType>::iterator prevIt = objDistancePairs.end();
        for(typename std::vector<internalType>::iterator it=objDistancePairs.begin(); it!= objDistancePairs.end(); ++it){
            typename std::set<typename Node<object_data_type>::iterator>::iterator deleteLoc = transientSet.find((*it).second);
            if(!(deleteLoc == transientSet.end())){
                double tempArea = node->getArea(*((*deleteLoc)));
                if((*prevIt).second == (*deleteLoc)){
                    negativeArea+=tempArea;
                    ++negCounter;
                    positiveArea-=tempArea;
                    --posCounter;
                }else{
                    --posCounter;
                    positiveArea-=tempArea;}
                transientSet.erase(deleteLoc);
            }else{
                if(!transientSet.empty()){
                    double tempArea = node->getArea(*((*(prevIt)).second));
                    negativeArea+=tempArea;
                    ++negCounter;}
                assert((transientSet.insert((*it).second)).second);}
            negP = (double)negativeArea/(double)totalArea;
            posP = (double)positiveArea/(double)totalArea;
            hValue = _tt + posP*posCounter*_ti + negP*negCounter*_ti;
            if(min > hValue){
                min = hValue;
                bestD = (*it).first;}
            prevIt = it;}
        return bestD;}

public:
    KdTree(double ti = 100, double tt = 500, int depth = 15, int minObjs = 2):_root(NULL),_ti(ti), _tt(tt),_depth(depth),_minObjs(minObjs){}
    bool buildTree(std::vector<Geometry*>::const_iterator beginObjectsIt, std::vector<Geometry*>::const_iterator endObjectsIt){
        if(_root!=NULL)
            return false;
        _root = new Node<object_data_type>();

        while(beginObjectsIt!=endObjectsIt){
            _root->getBoundingBox().merge((*beginObjectsIt)->getBoundingBox());
            _root->addObject((*beginObjectsIt)->getBoundingBox());
            ++beginObjectsIt;}
        splitNode(_root, _depth, _minObjs);}

    node_pointer splitNode(node_pointer node, int depth, int minObjs){
        unsigned int currNumObjs = node->getNumObjects();
        if(currNumObjs<=minObjs || depth < 0)
            return node;
        node_pointer positiveNode = new Node<object_data_type>();
        node_pointer negativeNode = new Node<object_data_type>();
        double xH = computeH(node,0);
        double yH = computeH(node,1);
        double zH = computeH(node,2);
        node->setPlaneDist(xH);
        node->setPlaneNormal(Vec3d(1.0f,0.0f,0.0f));
        int dim = 0;
        double dMin = xH;
        if(yH<dMin){
            node->setPlaneNormal(Vec3d(0.0f,1.0f,0.0f));
            node->setPlaneDist(yH);
            dim = 1;
            dMin = yH;}
        if(zH<dMin){
            node->setPlaneNormal(Vec3d(0.0f,0.0f,1.0f));
            node->setPlaneDist(zH);
            dim = 2;
            dMin = zH;}
        typename Node<object_data_type>::iterator it = node->getBeginIterator();
        while(it!=node->getEndIterator()){
            Vec3d normal(0.0f,0.0f,0.0f);
            double d1;
            double d2;
            (*it).getPlaneNormsDists(dim, normal, d1, d2);
            if(dMin>d2){
                negativeNode->getBoundingBox().merge(*it);
                negativeNode->addObject(*it);
            } else if(dMin<d1){
                positiveNode->getBoundingBox().merge(*it);
                positiveNode->addObject(*it);
            } else {
                negativeNode->getBoundingBox().merge(*it);
                negativeNode->addObject(*it);
                positiveNode->getBoundingBox().merge(*it);
                positiveNode->addObject(*it);}}
        node->_positiveHalf = splitNode(positiveNode, depth - 1, minObjs);
        node->_negativeHalf = splitNode(negativeNode, depth - 1, minObjs);}

    void deleteTree(){
        if(_root == NULL)
            return;
        delete _root;}};



#endif // KDTREE_H
