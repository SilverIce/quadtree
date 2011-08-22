#include "QuadTree.h"
#include <stdio.h>
#include <math.h>
#include <conio.h>
#include <vector>

struct TreeVisitor 
{
    QuadTree* tree;

    void operator()(const QuadTree::Node * node, uint32 addr)
    {
        printf("visiting adress: %u, absolute: %u\n", addr, uint32(node-tree->nodes));
    }
};

#define check(expr)\
    if ((bool)(expr) == false)\
        printf("exrpession " #expr " failed\n");

void QuadTree::debugSelf()
{
    uint8 depth = 10;
    check(  QuadTree::NodesPerLevelAmount(depth) == (uint64)pow(2.f, (int)(2*depth)) );
    check(  QuadTree::NodesAmount(depth) == (uint64) ((pow(2.f,(int)(2*depth+2))) -1u) / 3u );

    Node * end = this->nodes + NodesAmount(this->m_depth);
    for (Node * i = this->nodes; i != end; ++i)
        check(i->xDiv != DBG_WORD);

    check((end - this->nodes) == QuadTree::NodesAmount(this->m_depth));

    std::vector<QuadTree::Node> array(this->nodes, end);
}

void testDivision()
{
    SpaceDivision div = {100, 100};

    Circle c = {99, 99, 2};
    AABox2d box = AABox2d::create(c);

    check( div.intersectionQuadrants(box) == div.intersection(box));
}

int main()
{
    testDivision();

    uint32 MAP_SIZE = uint32(64.f * 533.333f);

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
