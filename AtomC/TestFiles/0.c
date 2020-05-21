
int sum()
{
	int	 i,v[5],s;
	s=0;
	for(i=0;i<5;i=i+1){
		v[i]=i;
		s=s+v[i];
		}
	return s;
}

void main()
{
	int		i1,s1;
	for(i1=0;i1<1000000;i1=i1+1)
	s1=sum();
	put_i(s);
}

