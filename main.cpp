#include "QuadTree.h"
#include <stdio.h>
#include <math.h>
#include <conio.h>

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

void CheckQuadFormulas()
{
    uint32 depth = 2;

    check(  QuadTree::NodesPerLevelAmount(depth) == (uint32)pow(2.f, (int)(2*depth)) );
    check(  QuadTree::NodesAmount(depth) == (uint32) ((pow(2.f,(int)(2*depth+2))) -1) / 3u );
    check(  QuadTree::NodesSidePerLevelAmount(depth) == (uint32)pow(2.f, (int)depth) );

}


int main()
{
    CheckQuadFormulas();

    uint32 MAP_SIZE = uint32(64.f * 533.333f);

    Point center = {MAP_SIZE/2, MAP_SIZE/2};
    QuadTree tree(4, center, MAP_SIZE);

    Circle c = {MAP_SIZE/2, MAP_SIZE/2, 600};
    TreeVisitor visitor = {&tree};
    tree.intersectRecursive(AABox2d::create(c), visitor);

    _getch();
    return 0;
}
