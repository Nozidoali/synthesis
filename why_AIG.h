#ifndef WHY_AIG
#define WHY_AIG
#include<stdio.h>
#include<algorithm>
#include<cmath>
#include<cstring>
struct literal
{
	string name;
	bool value;
};
struct why_AIG_node
{
	char type;
	bool FLAG;
	bool value;
	why_AIG_node * left, * right, *up;
	bool sign_left, sign_right;
	int depth;
	bool Update();

	why_AIG_node(){}
	~why_AIG_node(){}
};
struct why_AIG
{
	why_AIG_node * output;

	void Print_truth_table();
};
why_AIG_node * BuildTree()
{
	why_AIG_node * ret = new why_AIG_node;
	ret->left = ret->right = ret->up = NULL;
	ret->sign_left = ret->sign_right = true;
	ret->type = 'r';
	ret->depth = 0;
	return ret;
}
why_AIG_node * combine(why_AIG_node * x, why_AIG_node * y, bool x_sign, bool y_sign)
{
	why_AIG_node * ret = new why_AIG_node;
	ret->left = x, ret->right = y;
	x->up = y->up = ret;
	ret->sign_left = x_sign, ret->sign_right = y_sign;
	ret->type = 'r';
	ret->depth = std::max(x->depth,y->depth);
	return ret;
}
bool why_AIG_node :: Update()
{
	if(this->FLAG==true)
		return this->value;
	this->value = (!(Update(x->left)^x->sign_left)) & (!(UPdate(x->right)^x->sign_right));
	this->FLAG = true;
	return this->value;
}
void why_AIG :: Print_truth_table()
{
	
}
#endif
