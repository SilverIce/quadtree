#pragma once
#include <memory.h>

typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned char uint8;


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

enum ChildOffset{
    LeftUpper  = 0,
    RightUpper = 1,
    LeftLower  = 2,
    RightLower = 3,
};

enum QuadrantFlags{
    NorthWest  = 1 << LeftUpper,
    NorthEast  = 1 << RightUpper,
    SouthWest  = 1 << LeftLower,
    SouthEast  = 1 << RightLower,
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

        Mask_LeftUpper   = Left|Upper,   // 9
        Mask_RightUpper  = Right|Upper,  // 10
        Mask_LeftLower   = Left|Lower,   // 5
        Mask_RightLower  = Right|Lower,  // 6
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

    QuadrantFlags intersectionQuadrants(const AABox2d& p) const
    {
        if (xDiv < p.lo.x) //right
        {
            if (yDiv < p.lo.y)
                return NorthEast;
            else if (yDiv > p.hi.y)
                return SouthEast;
            else
                return (QuadrantFlags)(NorthEast|SouthEast);
        }
        else if (xDiv > p.hi.x) // left
        {
            if (yDiv < p.lo.y)
                return NorthWest;
            else if (yDiv > p.hi.y)
                return SouthWest;
            else
                return (QuadrantFlags)(NorthWest|SouthWest);
        }
        else //right | left
        {
            if (yDiv < p.lo.y)
                return (QuadrantFlags)(NorthEast|NorthWest);
            else if (yDiv > p.hi.y)
                return (QuadrantFlags)(SouthEast|SouthWest);
            else
                return (QuadrantFlags)(SouthEast|SouthWest | NorthEast|NorthWest);
        }
    }
};

class QuadTree
{
public:

    // calculates total amount of nodes for given tree depth
    static uint64 NodesAmount(uint8 depth)
    {
        return ((uint64(1) << (2*depth+2)) - 1) / 3;
    }

    // calculates amount of nodes for given level(depth)
    static uint64 NodesPerLevelAmount(uint8 depth)
    {
        return (uint64(1) << (2*depth));
    }

    typedef SpaceDivision Node;

    enum{
        DBG_WORD = 0,
        //MaxUInt32Depth = 14,
        //MaxUInt64Depth = 30,
    };

    static QuadTree* create(uint8 Depth, const Point& center, uint32 sideSize)
    {
        uint64 nodesAmount = NodesAmount(Depth);
        if (nodesAmount > ((uint64(1) << 32) - 1))  // nodes amount is greater than maximum unsigned 32-bit value
            return 0;

        QuadTree * tree = new QuadTree();
        tree->m_depth = Depth;
        tree->nodes = new Node[(uint32)nodesAmount];
        tree->initNodes(center, sideSize, (uint32)nodesAmount);
        return tree;
    }

    ~QuadTree() { delete[] nodes;}

    Node * Nodes() const { return nodes;}
    uint8 Depth() const { return m_depth;}

    void debugSelf();

    template<class T> void intersect(const AABox2d& p, T& visitor) const;

    template<class T> void intersectRecursive(const AABox2d& p, T& visitor) const;

    struct QuadIterator deepestContaining(const AABox2d& p) const;

private:
    explicit QuadTree() {}
    void initNodes(const Point& center, uint32 sideSize, uint32 nodesAmount);

    Node * nodes;
    uint8 m_depth;
};

struct QuadIterator
{
    QuadTree::Node * my_table;
    uint32 my_adress;
    uint8 myDepth;
    uint8 lastDepth;

    static QuadIterator create(const QuadTree * tree)
    {
        QuadIterator it = {tree->Nodes(), (uint32)0, (uint8)0, tree->Depth()};
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

    QuadIterator nearby(ChildOffset offset_) const
    {
        QuadIterator it = *this;
        it.moveTo(offset_);
        return it;
    }

    void moveTo(ChildOffset offset_)
    {
        my_adress = my_adress/4*4 + offset_;
    }

    ChildOffset offset() const
    {
        return (ChildOffset)(my_adress % 4);
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
                return;

            visitor(it);

            if (!it.moveNext())   // last node has no childs, no sense test for intersection
                return;

            QuadrantFlags res = me->intersectionQuadrants(p);

            if (res & (1 << LeftUpper))
                Visit(it.nearby(LeftUpper), p, visitor);
            if (res & (1 << RightUpper))
                Visit(it.nearby(RightUpper), p, visitor);
            if (res & (1 << LeftLower))
                Visit(it.nearby(LeftLower), p, visitor);
            if (res & (1 << RightLower))
                Visit(it.nearby(RightLower), p, visitor);
        }
    };

    TT::Visit(QuadIterator::create(this), p, visitor);
}

void QuadTree::initNodes(const Point& center, uint32 sideSize, uint32 nodesAmount)
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
    memset(nodes, DBG_WORD, nodesAmount * sizeof Node);
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
        case SpaceDivision::Mask_LeftUpper:
            it.moveTo(LeftUpper);
            continue;
        case SpaceDivision::Mask_RightUpper:
            it.moveTo(RightUpper);
            continue;
        case SpaceDivision::Mask_LeftLower:
            it.moveTo(LeftLower);
            continue;
        case SpaceDivision::Mask_RightLower:
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

    for(;;)
    {
        const Node * me = it.current();
        if ( (uint32&)(*me) == DBG_WORD )
            return;

        visitor(it);

        if (it.moveNext())
        {
            if ((stack - stackArray) == StackLast)
                return; // overflow

            *(++stack) = (uint8)me->intersectionQuadrants(p);
        }
        else
        {
            for(;;)
            {
                if (stack == stackArray)
                    return;

                if (*stack)
                    break;

                --stack;
                it.movePrev();
            }
        }

        uint8& res = *stack;
        // couple of 'if' statements are faster than commented code below
        if (res & NorthWest){
            res &= ~NorthWest;
            it.moveTo(LeftUpper);
        }
        else if (res & NorthEast){
            res &= ~NorthEast;
            it.moveTo(RightUpper);
        }
        else if (res & SouthWest){
            res &= ~SouthWest;
            it.moveTo(LeftLower);
        }
        else if (res & SouthEast){
            res &= ~SouthEast;
            it.moveTo(RightLower);
        }
        /*for (uint8 i = 0; i < 4; ++i)
        {
            if (res & (1 << i))
            {
                res &= ~(1 << i);
                it.moveTo((ChildOffset)i);
                break;
            }
        }*/
    }
}
