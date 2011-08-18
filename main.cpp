#include "QuadTree.h"
#include <stdio.h>

struct TreeVisitor 
{
    void operator()(QuadTree::Node * node, uint32 addr)
    {
        printf("visiting adress: %u\n", addr);
    }
};

int main()
{
    uint32 MAP_SIZE = uint32(64.f * 533.333f);

    Point center = {MAP_SIZE/2, MAP_SIZE/2};
    QuadTree tree(4, center, MAP_SIZE);

    //Circle c = {MAP_SIZE/2, MAP_SIZE/2, 100};
    //tree.intersectRecursive(AABox2d::create(c), TreeVisitor());

    return 0;
}
