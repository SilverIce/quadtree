#pragma once
#include <memory.h>

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

        LeftUpper   = Left|Upper,   // 9
        RightUpper  = Right|Upper,  // 10
        LeftLower   = Left|Lower,   // 5
        RightLower  = Right|Lower,  // 6
    };

    enum Quadrants{
        NorthEast   = 0x1,
        SouthEast   = 0x2,
        SouthWest   = 0x4,
        NorthWest   = 0x8,
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
            result |= Upper;
        else if (yDiv > p.hi.y)
            result |= Lower;
        else
            result |= (Lower|Upper);
        return (IntersectionResult)result;
    }

    Quadrants intersectionQuadrants(const AABox2d& p) const
    {
        if (xDiv < p.lo.x) //right
        {
            if (yDiv < p.lo.y)
                return NorthEast;
            else if (yDiv > p.hi.y)
                return SouthEast;
            else
                return (Quadrants)(NorthEast|SouthEast);
        }
        else if (xDiv > p.hi.x) // left
        {
            if (yDiv < p.lo.y)
                return NorthWest;
            else if (yDiv > p.hi.y)
                return SouthWest;
            else
                return (Quadrants)(NorthWest|SouthWest);
        }
        else //right | left
        {
            if (yDiv < p.lo.y)
                return (Quadrants)(NorthEast|NorthWest);
            else if (yDiv > p.hi.y)
                return (Quadrants)(SouthEast|SouthWest);
            else
                return (Quadrants)(SouthEast|SouthWest | NorthEast|NorthWest);
        }
    }
};

class QuadTree
{
public:

   // calculates total amount of nodes for given tree depth
    static uint32 NodesAmount(uint32 depth)
    {
        return ((1 << (2*depth+2)) - 1) / 3;
    }

    // calculates amount of nodes for given level(depth)
    static uint32 NodesPerLevelAmount(uint32 depth)
    {
        return (1 << (2*depth));
    }

    typedef SpaceDivision Node;
    /*struct Node
    {
        SpaceDivision div;
       // uint32 nodeId;
    };*/

    enum{
        DBG_WORD = 0,
    };

    explicit QuadTree(uint8 Depth, const Point& center, uint32 sideSize) : m_depth(Depth), nodes(0)
    {
        nodes = new Node[NodesAmount(m_depth)];
        initNodes(center, sideSize);
    }

    ~QuadTree() { delete[] nodes;}

    void debugSelf();

    void initNodes(const Point& center, uint32 sideSize);

    template<class T> void intersect(const AABox2d& p, T& visitor) const;

    template<class T> void intersectRecursive(const AABox2d& p, T& visitor) const;

    struct QuadIterator deepestContaining(const AABox2d& p) const;

    Node * nodes;
    uint8 m_depth;
};

enum ChildOffset{
    LeftUpper,
    RightUpper,
    LeftLower,
    RightLower,
};

struct QuadIterator
{
    QuadTree::Node * my_table;
    uint32 my_adress;
    uint8 myDepth;
    uint8 lastDepth;

    static QuadIterator create(const QuadTree * tree)
    {
        QuadIterator it = {tree->nodes, (uint32)0, (uint8)0, tree->m_depth};
        return it;
    }

    bool moveNext()
    {
        if (myDepth == lastDepth)
            return false;

        my_table += QuadTree::NodesPerLevelAmount(myDepth);
        ++myDepth;
        my_adress *= 4;
        return true;
    }

    bool movePrev()
    {
        if (myDepth == 0)
            return false;

        my_adress /= 4;
        --myDepth;
        my_table -= QuadTree::NodesPerLevelAmount(myDepth);
        return true;
    }

    QuadIterator nearby(ChildOffset offset) const
    {
        QuadIterator it = *this;
        it.my_adress += offset;
        return it;
    }

    void moveTo(ChildOffset offset)
    {
        my_adress = my_adress/4*4 + offset;
    }

    QuadTree::Node* current() const { return my_table + my_adress;}
};

template<class T> void QuadTree::intersectRecursive(const AABox2d& p, T& visitor) const
{
    struct TT {
        static void Visit(QuadIterator it, const AABox2d& p, T& visitor)
        {
            const Node * me = it.current();

            if ( (uint32&)(*me) == DBG_WORD )
            {
                return;
            }

            visitor(me, it.my_adress);

            if (!it.moveNext())   // last node has no childs
                return;

            SpaceDivision::IntersectionResult res = me->intersection(p);

            if ((res & SpaceDivision::LeftUpper) == SpaceDivision::LeftUpper)
                Visit(it.nearby(LeftUpper), p, visitor);
            if ((res & SpaceDivision::RightUpper) == SpaceDivision::RightUpper)
                Visit(it.nearby(RightUpper), p, visitor);
            if ((res & SpaceDivision::LeftLower) == SpaceDivision::LeftLower)
                Visit(it.nearby(LeftLower), p, visitor);
            if ((res & SpaceDivision::RightLower) == SpaceDivision::RightLower)
                Visit(it.nearby(RightLower), p, visitor);
        }
    };

    TT::Visit(QuadIterator::create(this), p, visitor);
}

void QuadTree::initNodes(const Point& center, uint32 sideSize)
{
    struct TT  {
        static void Visit(QuadIterator it, Point myCenter, uint32 mySideSize)
        {
            Node * me = it.current();

            if ( (uint32&)(*me) != DBG_WORD )
                return;

            me->xDiv = myCenter.x;
            me->yDiv = myCenter.y;

            if (!it.moveNext())   // last node has no childs
                return;

            Point child_center(myCenter);
            uint32 child_SideSize = mySideSize / 2;
            uint32 offset = mySideSize / 4;

            child_center.x -= offset;
            child_center.y += offset;
            Visit(it.nearby(LeftUpper), child_center, child_SideSize); // LeftUpper

            child_center = myCenter;
            child_center.x += offset;
            child_center.y += offset;
            Visit(it.nearby(RightUpper), child_center, child_SideSize); // RightUpper

            child_center = myCenter;
            child_center.x -= offset;
            child_center.y -= offset;
            Visit(it.nearby(LeftLower), child_center, child_SideSize); // LeftLower

            child_center = myCenter;
            child_center.x += offset;
            child_center.y -= offset;
            Visit(it.nearby(RightLower), child_center, child_SideSize); // RightLower
        }
    };
    memset(nodes, DBG_WORD, NodesAmount(m_depth) * sizeof Node);
    TT::Visit(QuadIterator::create(this), center, sideSize);
}

QuadIterator QuadTree::deepestContaining(const AABox2d& p) const
{
    QuadIterator it(QuadIterator::create(this));

    for(;;)
    {
        Node * me = it.current();

        if ( (uint32&)(*me) == DBG_WORD )
            break;

        if (!it.moveNext())   // last node has no childs
            break;

        SpaceDivision::IntersectionResult res = me->intersection(p);
        switch (res)
        {
        case SpaceDivision::LeftUpper:
            it.moveTo(LeftUpper);
            continue;
        case SpaceDivision::RightUpper:
            it.moveTo(RightUpper);
            continue;
        case SpaceDivision::LeftLower:
            it.moveTo(LeftLower);
            continue;
        case SpaceDivision::RightLower:
            it.moveTo(RightLower);
            continue;
        default:
            break;
        }
    }
    return it;
}

template<class T> void QuadTree::intersect(const AABox2d& p, T& visitor) const
{
    enum{
        StackLast = 30,
    };

    QuadIterator it(QuadIterator::create(this));
    uint8 stackArray[StackLast+1];
    uint8 * stack = stackArray;
    uint8 * stackBottom = stackArray;
    *stackBottom = 0;

    for(;;)
    {
        const Node * me = it.current();

        if ( (uint32&)(*me) == DBG_WORD )
            return;

        visitor(me, it.my_adress);

        if (it.moveNext())
        {
            uint8 res = (uint8)me->intersectionQuadrants(p);
            if (res & SpaceDivision::NorthWest){
                res &= ~SpaceDivision::NorthWest;
                it.moveTo(LeftUpper);
            }
            else if (res & SpaceDivision::NorthEast){
                res &= ~SpaceDivision::NorthEast;
                it.moveTo(RightUpper);
            }
            else if (res & SpaceDivision::SouthWest){
                res &= ~SpaceDivision::SouthWest;
                it.moveTo(LeftLower);
            }
            else if (res & SpaceDivision::SouthEast){
                res &= ~SpaceDivision::SouthEast;
                it.moveTo(RightLower);
            }

            if ((stack - stackBottom) == StackLast)
                return; // overflow

            *(++stack) = res;
        }
        else
        {
            for(;;)
            {
                if (stack == stackBottom)
                    return;

                if (*stack)
                    break;

                --stack;
                it.movePrev();
            }

            uint8& res = *stack;

            if (res & SpaceDivision::NorthWest){
                res &= ~SpaceDivision::NorthWest;
                it.moveTo(LeftUpper);
            }
            else if (res & SpaceDivision::NorthEast){
                res &= ~SpaceDivision::NorthEast;
                it.moveTo(RightUpper);
            }
            else if (res & SpaceDivision::SouthWest){
                res &= ~SpaceDivision::SouthWest;
                it.moveTo(LeftLower);
            }
            else if (res & SpaceDivision::SouthEast){
                res &= ~SpaceDivision::SouthEast;
                it.moveTo(RightLower);
            }
        }
    }
}
