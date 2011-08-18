#pragma once

typedef unsigned int uint32;
typedef unsigned long uint64;
typedef unsigned char uint8;

/*
    Симметричное quad-tree. Благодаря симметричности рассчитать адрес точки очень легко.
    Ветви такого дерева располагаются/должны располагаться в одном куске памяти

    value >> N bits


*/

// Class-adress
class QuadAdress
{
    uint32 x;
    uint32 y;
    uint32 depth;   //really needed?

    explicit QuadAdress(uint32 X, uint32 Y, uint32 Depth) : x(X), y(Y), depth(Depth) {}

    // returns adress of parent cell
    QuadAdress base() const
    {
        return QuadAdress(x/2, y/2, depth-1);
    }  
};

struct Point
{
    uint32 x, y;
};

struct Circle
{
    Point point;
    uint32 radius;
};

struct AABox2d
{
    Point lo;
    Point hi;

    static AABox2d create(const Circle& c)
    {
        AABox2d box = {
            {c.point.x-c.radius, c.point.y-c.radius},
            {c.point.x+c.radius, c.point.y+c.radius}
        };
        return box;
    }
};

// divides space into four subspaces
struct SpaceDivision 
{
    uint32 xDiv, yDiv;

    enum IntersectionResult{
        None        = 0x0,
        Left        = 0x1,
        Right       = 0x2,
        Lower       = 0x4,
        Upper       = 0x8,

        LeftUpper   = Left|Upper,   //3
        RightUpper  = Right|Upper,
        LeftLower   = Left|Lower,
        RightLower  = Right|Lower,
    };

    /*IntersectionResult intersection(const Point& p) const
    {
        IntersectionResult result = (p.x >= xDiv) ? Right : Left;
        result |= (p.y >= yDiv) ? Upper : Lower;
        return result;
    }*/

    IntersectionResult intersection(const Circle& p) const
    {
        return intersection(AABox2d::create(p));
    }

    IntersectionResult intersection(const AABox2d& p) const
    {
        uint8 result(None);

        if (xDiv < p.lo.x)
            result |= Right;
        else if (xDiv > p.hi.x)
            result |= Left;
        else
            result |= (Left|Right);

        if (yDiv < p.lo.y)
            result |= Lower;
        else if (yDiv > p.hi.y)
            result |= Upper;
        else
            result |= (Lower|Upper);
        return (IntersectionResult)result;
    }
};

class QuadTree
{
    // calculates total amount of nodes for given tree depth
    static uint32 NodesAmount(uint32 depth)
    {
        return ((2 << (2*depth)) - 1) / 3;
    }

    // calculates amount of nodes for given level(depth)
    static uint32 NodesPerLevelAmount(uint32 depth)
    {
        return (2 << (2*depth - 2));
    }

    // calculates amount of nodes for level(depth)
    static uint32 NodesSidePerLevelAmount(uint32 depth)
    {
        return (2 << (depth - 1));
    }

public:

    struct Node
    {
        SpaceDivision div;
       // uint32 nodeId;
    };

    explicit QuadTree(uint32 Depth, const Point& center, uint32 sideSize) : depth(Depth), nodes(0)
    {
        uint32 nodesCount = NodesAmount(depth);
        nodes = new Node[nodesCount];
        initNodes(center, sideSize);
    }

    ~QuadTree() { delete[] nodes;}

    void initNodes(const Point& center, uint32 sideSize)
    {
        struct TT 
        {
            static void Visit(Node * my_table, Point myCenter, uint32 mySideSize, uint32 lastDepth, uint32 myDepth, uint32 my_adress)
            {
                Node * me = my_table + my_adress;

                me->div.xDiv = myCenter.x;
                me->div.yDiv = myCenter.y;

                if (myDepth == lastDepth)   // last node has no childs
                    return;

                Node * child_table = my_table + NodesPerLevelAmount(myDepth);
                uint32 child_depth = myDepth + 1;
                uint32 child_adress = my_adress * 4;
                uint32 child_levelSideSize = NodesSidePerLevelAmount(child_depth);

                Point child_center(myCenter);
                uint32 child_SideSize = mySideSize / 2;
                uint32 offset = mySideSize / 4;

                child_center.x -= offset;
                child_center.y += offset;
                Visit(child_table, child_center, child_SideSize, lastDepth, child_depth, child_adress); // LeftUpper

                child_center = myCenter;
                child_center.x += offset;
                child_center.y += offset;
                Visit(child_table, child_center, child_SideSize, lastDepth, child_depth, child_adress+1); // RightUpper

                child_center = myCenter;
                child_center.x -= offset;
                child_center.y -= offset;
                Visit(child_table, child_center, child_SideSize, lastDepth, child_depth, child_adress+child_levelSideSize); // LeftLower

                child_center = myCenter;
                child_center.x -= offset;
                child_center.y += offset;
                Visit(child_table, child_center, child_SideSize, lastDepth, child_depth, child_adress+child_levelSideSize+1); // RightLower
            }
        };
        TT::Visit(nodes, center, sideSize, depth, 1, 0);
    }

    // Not recursive. Hard to implement
    void intersect(const AABox2d& p)
    {
        uint32 I = 1;
        uint32 stack[0x100];
        uint32 stackItr = 0;

        /*
            посетить рут, записать 2(например) чайлда
        */

        {
            uint32 levelSize = NodesPerLevelAmount(I);
            Node * node = nodes + I;
            SpaceDivision::IntersectionResult res = node->div.intersection(p);


            if ((res & SpaceDivision::LeftUpper) == SpaceDivision::LeftUpper)
                stack[stackItr++] = ++I;
            if ((res & SpaceDivision::RightUpper) == SpaceDivision::RightUpper)
                stack[stackItr++] = ++I;
            if ((res & SpaceDivision::LeftLower) == SpaceDivision::LeftLower)
                stack[stackItr++] = ++I;
            if ((res & SpaceDivision::RightLower) == SpaceDivision::RightLower)
                stack[stackItr++] = ++I;


            ++I;
            ;
        }
    }

    template<class T> void intersectRecursive(const AABox2d& p, T& visitor)
    {
        struct TT 
        {
            static void Visit(Node * my_table, const AABox2d& p, T& visitor, uint32 lastDepth, uint32 myDepth = 1, uint32 my_adress = 0)
            {
                Node * me = my_table + my_adress;

                visitor(me, my_adress);

                if (myDepth == lastDepth)   // last node has no childs
                    return;

                SpaceDivision::IntersectionResult res = me->div.intersection(p);

                Node * child_table = my_table + NodesPerLevelAmount(myDepth);
                uint32 child_depth = myDepth + 1;
                uint32 child_adress = my_adress * 4;

                uint32 child_levelSideSize = NodesSidePerLevelAmount(child_depth);

                if ((res & SpaceDivision::LeftUpper) == SpaceDivision::LeftUpper)
                    Visit(child_table, p, visitor, lastDepth, child_depth, child_adress);
                if ((res & SpaceDivision::RightUpper) == SpaceDivision::RightUpper)
                    Visit(child_table, p, visitor, lastDepth, child_depth, child_adress+1);
                if ((res & SpaceDivision::LeftLower) == SpaceDivision::LeftLower)
                    Visit(child_table, p, visitor, lastDepth, child_depth, child_adress+child_levelSideSize);
                if ((res & SpaceDivision::RightLower) == SpaceDivision::RightLower)
                    Visit(child_table, p, visitor, lastDepth, child_depth, child_adress+child_levelSideSize+1);
            }
        };

        TT::Visit(nodes, p, visitor, depth);
    }

    Node * nodes;
    uint32 depth;
};
