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
    uint32 depth = 10;
    check(  QuadTree::NodesPerLevelAmount(depth) == (uint32)pow(2.f, (int)(2*depth)) );
    check(  QuadTree::NodesAmount(depth) == (uint32) ((pow(2.f,(int)(2*depth+2))) -1u) / 3u );
    check(  QuadTree::NodesSidePerLevelAmount(depth) == (uint32)pow(2.f, (int)depth) );

    Node * end = this->nodes + NodesAmount(this->m_depth);
    for (Node * i = this->nodes; i != end; ++i)
        check(i->xDiv != DBG_WORD);

    check((end - this->nodes) == (int)QuadTree::NodesAmount(this->m_depth));

    std::vector<QuadTree::Node> array(this->nodes, end);
}


int main()
{
    uint32 MAP_SIZE = uint32(64.f * 533.333f);

    Point center = {MAP_SIZE/2, MAP_SIZE/2};
    QuadTree tree(4, center, MAP_SIZE);

    tree.debugSelf();

    Circle c = {MAP_SIZE/2, MAP_SIZE/2, 600};
    TreeVisitor visitor = {&tree};
    tree.intersectRecursive(AABox2d::create(c), visitor);

    QuadIterator it = tree.deepestContaining(AABox2d::create(c));

    _getch();
    return 0;
}
