#ifndef KDTREE_H
#define KDTREE_H
#include "scene/bbox.h"
#include <vector>
#include"scene/scene.h"
#include <algorithm>
#include <utility>
#include <cassert>
#include <set>
#include <stack>
//#include "scene/ray.h"

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
    typedef T* object_pointer;
    typedef typename std::vector<object_pointer>::const_iterator iterator;
    typedef Node<T>* node_pointer;
    node_pointer _positiveHalf;
    node_pointer _negativeHalf;
private:
    Vec3d _splittingPlaneNormal;
    double _splittingPlaneDist;
    BoundingBox _box;
    std::vector<object_pointer> _objects;
public:
    const Vec3d& getSplittingPlaneNormal(){
        return _splittingPlaneNormal;}
    const double& getSplittingPlaneDist(){
        return _splittingPlaneDist;}

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

    bool isLeaf(){
        return ((_positiveHalf==NULL)&&(_negativeHalf==NULL));}

    void setPlaneDist(double d){
        _splittingPlaneDist = d;}

    void setPlaneNormal(Vec3d n){
        _splittingPlaneNormal = n;}

    BoundingBox& getBoundingBox(){
        return _box;}

    void addObject(object_pointer obj){
        this->_box.merge(obj->getBoundingBox());
        _objects.push_back(obj);}

    double getArea(object_pointer obj){
        BoundingBox tempBox = obj->getBoundingBox();
        return tempBox.area();}

    double getBoxArea(){
        return _box.area();}

    double getObjectsArea(){
        double toReturn = 0.0f;
        for(iterator it = _objects.begin(); it!=_objects.end(); ++it){
            assert(!((*it)->getBoundingBox().isEmpty()));
            BoundingBox tempBox = (*it)->getBoundingBox();
            toReturn += tempBox.area();}
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
    typedef T* object_pointer;
    typedef typename std::vector<T*>::const_iterator object_pointer_iterator;
    typedef typename Node<object_data_type>::node_pointer node_pointer;
private:
    double _ti, _tt;
    int _depth;
    int _minObjs;
    node_pointer _root;
    double computeH(Node<object_data_type>* node, int dim){
        if(!node->hasObjects()){
            return -1.0f;}
        double totalArea = node->getBoxArea();
        double min = 1.0e308; // 1.0e308 is close to infinity... close enough for us!
        double bestD = 0.0f;
        std::vector<std::pair<double, typename Node<object_data_type>::iterator> > objDistancePairs;
        objDistancePairs.reserve(2*node->getNumObjects());

        for(typename Node<object_data_type>::iterator it = node->getBeginIterator(); it!=node->getEndIterator(); ++it){
            assert(!((*it)->getBoundingBox().isEmpty()));
            double d1;
            double d2;
            Vec3d normal(0.0f,0.0f,0.0f);
            (*it)->getBoundingBox().getPlaneNormsDists(dim, normal, d1, d2);
            objDistancePairs.push_back(std::make_pair(d1,it));
            objDistancePairs.push_back(std::make_pair(d2,it));}

        typedef std::pair<double, typename Node<object_data_type>::iterator > internalType;
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
            (*it)->getBoundingBox().getPlaneNormsDists(dim, normal, d1, d2);
            if(dMin>d2){
                negativeNode->addObject(*it);
            } else if(dMin<d1){
                positiveNode->addObject(*it);
            } else {
                negativeNode->addObject(*it);
                positiveNode->addObject(*it);}
            ++it;}
        node->_positiveHalf = splitNode(positiveNode, depth - 1, minObjs);
        node->_negativeHalf = splitNode(negativeNode, depth - 1, minObjs);
        return node;}

    struct stackElement{
        stackElement(){
            node = NULL;
            tMin = 1.0e308;
            tMax = -1.0e308;}
        node_pointer node;
        double tMin;
        double tMax;};
public:
    KdTree(double ti = 100, double tt = 500, int depth = 100, int minObjs = 2):_root(NULL),_ti(ti), _tt(tt),_depth(depth),_minObjs(minObjs){}

    ~KdTree(){
        deleteTree();}

    bool buildTree(object_pointer_iterator beginObjectsIt, object_pointer_iterator endObjectsIt){
        if(_root!=NULL)
            return false;
        _root = new Node<object_data_type>();

        while(beginObjectsIt!=endObjectsIt){
            _root->addObject((*beginObjectsIt));
            ++beginObjectsIt;}
        splitNode(_root, _depth, _minObjs);
        return true;}

    void deleteTree(){
        if(_root == NULL)
            return;
        delete _root;}
    bool rayTreeTraversal(isect& i, const ray& r){
            double tMin=0.0f, tMax=0.0f, tPlane=0.0f;
            if(_root->getBoundingBox().intersect( r, tMin, tMax))
                return false;
            std::stack<stackElement> stack;
            node_pointer farChild, parent, nearChild;
            stackElement elem;
            elem.node = _root;
            elem.tMin = tMin;
            elem.tMax = tMax;
            stack.push(elem);
            while( !stack.empty() ){
                stackElement current = stack.top();
                stack.pop();
                parent = current.node;
                tMin = current.tMin;
                tMax = current.tMax;
                while (!parent->isLeaf()){
                    if(parent->getSplittingPlaneNormal()[0] == 1){
                        tPlane = parent->getSplittingPlaneDist() - r.getPosition()[0];
                        tPlane /= (double)r.getDirection()[0];
                    } else if(parent->getSplittingPlaneNormal()[1] == 1){
                        tPlane = parent->getSplittingPlaneDist() - r.getPosition()[1];
                        tPlane /= (double)r.getDirection()[1];
                    } else if(parent->getSplittingPlaneNormal()[2] == 1){
                        tPlane = parent->getSplittingPlaneDist() - r.getPosition()[2];
                        tPlane /= (double)r.getDirection()[2];}

                    if(tPlane > 0){
                        nearChild = parent->_negativeHalf;
                        farChild = parent->_positiveHalf;
                    } else {
                        nearChild = parent->_positiveHalf;
                        farChild = parent->_negativeHalf;}

                    if(tPlane >= tMax || tPlane <0){
                        parent = nearChild;
                    } else if(tPlane <= tMin){
                        parent = farChild;
                    } else {
                        stackElement tmp;
                        tmp.node = farChild;
                        tmp.tMin = tPlane;
                        tmp.tMax = tMax;
                        stack.push(tmp);
                        parent = nearChild;
                        tMax = tPlane;}}
                Vec3d closestPointIntersect (0.0f,0.0f,0.0f);
                object_pointer closestObject = NULL;
                isect minIntersection;
                bool haveOne = false;
                typename Node<object_data_type>::iterator it = parent->getBeginIterator();
                while (it != parent->getEndIterator()){
                    //calculate intersection
                    //check if it exists in boundbox
                    //check vs closest point
                    isect cur;
                    if( (*it)->intersect( r, cur )){
                        Vec3d pointOfintersect = r.at(cur.t );
                        if((*it)->getBoundingBox().intersects(pointOfintersect)){
                            if(!haveOne){
                                minIntersection = cur;
                                closestObject = *it;
                                closestPointIntersect = pointOfintersect;
                                haveOne = true;
                            } else if(minIntersection.t > cur.t){
                                minIntersection = cur;
                                closestObject = *it;
                                closestPointIntersect = pointOfintersect;}}}
                    ++it;}
                i = minIntersection;
                return true;}
            return false;}};



#endif // KDTREE_H
