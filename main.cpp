#include "QuadTree.h"
#include <stdio.h>
#include <math.h>
#include <conio.h>
#include <vector>

struct TreeVisitor 
{
    QuadTree* tree;

    void operator()(const QuadIterator& it)
    {
        printf("visiting adress: %u, absolute: %u\n", it.my_adress, uint32(it.current() - tree->Nodes()));
    }
};

#define check(expr)\
    if ((bool)(expr) == false)\
        printf("expression " #expr " failed\n");

void QuadTree::debugSelf()
{
    uint8 depth = 10;
    check(  QuadTree::NodesPerLevelAmount(depth) == (uint64)pow(2.f, (int)(2*depth)) );
    check(  QuadTree::NodesAmount(depth) == (uint64) ((pow(2.f,(int)(2*depth+2))) -1u) / 3u );

    Node * end = this->nodes + NodesAmount(this->m_depth);
    for (Node * i = this->nodes; i != end; ++i)
        check(i->xDiv != DBG_WORD);

    std::vector<QuadTree::Node> array(this->nodes, end);
}

void testDivision()
{
    SpaceDivision div = {100, 100};

    Circle c = {99, 99, 2};
    AABox2d box = AABox2d::create(c);

    check( div.intersectionQuadrants(box) == div.intersection(box));
}

class Timer
{
public:
    typedef unsigned __int64 rt_time;
    explicit Timer() { reset();}
    rt_time passed() const { return now() - start;}
    void reset() { start = now();}
private:
    static rt_time now()
    {
        __asm rdtsc;
    }
    rt_time start;
};

int main()
{
    testDivision();

    length_type MAP_SIZE = length_type(64.f * 533.333f);

    Point center = {MAP_SIZE/2, MAP_SIZE/2};
    QuadTree * tree = QuadTree::create(8, center, MAP_SIZE);
    if (!tree)
        return 0;

    tree->debugSelf();

    Circle c = {MAP_SIZE/2, MAP_SIZE/2, 1};
    TreeVisitor visitor = {tree};
    AABox2d box(AABox2d::create(c));
    tree->intersectRecursive(box, visitor);
    tree->intersect(box, visitor);



    QuadIterator it = tree->deepestContaining(AABox2d::create(c));

    _getch();
    return 0;
}
