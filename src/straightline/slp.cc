#include "straightline/slp.h"

#include <iostream>

namespace A
{
    int A::CompoundStm::MaxArgs() const
    {
        // TODO: put your code here (lab1).
        int arg1 = stm1->MaxArgs();
        int arg2 = stm2->MaxArgs();
        if(arg1>=arg2)return arg1;
        else return arg2;
    }
    int A::AssignStm::MaxArgs() const
    {
        // TODO: put your code here (lab1).
        return exp->MaxArgs();
    }
    int A::PrintStm::MaxArgs() const
    {
        int arg1 = exps->MaxArgs();
        int arg2 = exps->NumExps();
        // TODO: put your code here (lab1).
        if(arg1>=arg2)return arg1;
        else return arg2;
    }

    int A::IdExp::MaxArgs() const
    {
        return 0;
    }
    int A::NumExp::MaxArgs() const
    {
        return 0;
    }
    int A::OpExp::MaxArgs() const
    {
        int arg1 = left->MaxArgs();
        int arg2 = right->MaxArgs();
        if(arg1>=arg2)return arg1;
        else return arg2;
    }
    int A::EseqExp::MaxArgs() const
    {
        int arg1 = stm->MaxArgs();
        int arg2 = exp->MaxArgs();
        if(arg1>=arg2)return arg1;
        else return arg2;
    }

    int A::PairExpList::MaxArgs() const
    {
        int arg1 = exp->MaxArgs();
        int arg2 = tail->MaxArgs();
        if(arg1>=arg2)return arg1;
        else return arg2;
    }
    int A::LastExpList::MaxArgs() const
    {
        return exp->MaxArgs();
    }

    Table *A::CompoundStm::Interp(Table *t) const
    {
        // TODO: put your code here (lab1).
        Table *temp = stm1->Interp(t);
        return stm2->Interp(temp);
    }
    Table *A::AssignStm::Interp(Table *t) const
    {
        // TODO: put your code here (lab1).
        return t->Update(id, exp->Interp(t)->i);
    }
    Table *A::PrintStm::Interp(Table *t) const
    {
        // TODO: put your code here (lab1).
        return exps->Interp(t)->t;
    }

    IntAndTable *A::IdExp::Interp(Table *t) const
    {
        IntAndTable *temp = new IntAndTable(t->Lookup(id), t);
        return temp;
    }
    IntAndTable *A::NumExp::Interp(Table *t) const
    {
        IntAndTable *temp = new IntAndTable(num, t);
        return temp;
    }
    IntAndTable *A::OpExp::Interp(Table *t) const
    {
        IntAndTable *temp1 = left->Interp(t);
        // int id1 = temp1.t->Lookup(temp1.i, temp1.t);
        IntAndTable *temp2 = right->Interp(temp1->t);
        int num1 = temp1->i;
        int num2 = temp2->i;
        IntAndTable *ret;
        switch (oper)
        {
            case PLUS:
                ret = new IntAndTable(num1+num2,temp2->t);
                break;
            case MINUS:
                ret = new IntAndTable(num1-num2,temp2->t);
                break;
            case TIMES:
                ret = new IntAndTable(num1*num2,temp2->t);
                break;
            case DIV:
                ret = new IntAndTable(num1/num2,temp2->t);
                break;
        }
        return ret;
    }
    IntAndTable *A::EseqExp::Interp(Table *t) const
    {
        Table *temp = stm->Interp(t);
        IntAndTable *ret = exp->Interp(temp);
        return ret;
    }

    IntAndTable *A::LastExpList::Interp(Table *t) const {
        printf("%d\n",exp->Interp(t)->i);
        return exp->Interp(t);
    }
    IntAndTable *A::PairExpList::Interp(Table *t) const
    {
        IntAndTable *temp = exp->Interp(t);
        printf("%d\t",temp->i);
        return tail->Interp(temp->t);
    }

   int A::PairExpList::NumExps()
   {
       return 1+tail->NumExps();
   }
   int A::LastExpList::NumExps()
   {
       return 1;
   }

    int Table::Lookup(const std::string &key) const
    {
        if (id == key)
        {
            return value;
        }
        else if (tail != nullptr)
        {
            return tail->Lookup(key);
        }
        else
        {
            assert(false);
        }
    }

    Table *Table::Update(const std::string &key, int val) const
    {
        return new Table(key, val, this);
    }
} // namespace A
