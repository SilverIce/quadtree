
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

void QuadTree::initNodes(const Point& center, length_type sideSize, uint32 nodesAmount)
{
    struct TT  {
        static void Visit(QuadIterator it, Point myCenter, length_type mySideSize)
        {
            Node * me = it.current();

            if ( (uint32&)(*me) != DBG_WORD )
                return;

            me->xDiv = myCenter.x;
            me->yDiv = myCenter.y;

            if (!it.moveNext())   // last node has no childs
                return;

            Point child_center(myCenter);
            length_type child_SideSize = mySideSize / 2;
            length_type offset = mySideSize / 4;

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
        it.movePrev();
        break;
    }
    return it;
}

template<class T> void QuadTree::Deprecate_intersect(const AABox2d& p, T& visitor) const
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

template<class T> void QuadTree::intersect(const AABox2d& p, T& visitor) const
{
    enum{
        StackLast = 30,
    };
    uint8 stackArray[StackLast+1];
    uint8 * stack = stackArray;

    QuadIterator it(QuadIterator::create(this));
    visitor(it);

    for(;;)
    {
        const Node * me = it.current();
        if ( (uint32&)(*me) == DBG_WORD )
            return;

        if (it.moveNext())
        {
            if ((stack - stackArray) == StackLast)
                return; // overflow

            uint8 res = (uint8)me->intersectionQuadrants(p);
            *(++stack) = res;

            for (uint8 i = 0; i < 4; ++i)
            {
                if (res & (1 << i))
                {
                    it.moveTo((ChildOffset)i);
                    visitor(it);
                }
            }
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
        for (uint8 i = 0; i < 4; ++i)
        {
            if (res & (1 << i))
            {
                res &= ~(1 << i);
                it.moveTo((ChildOffset)i);
                break;
            }
        }
    }
}
