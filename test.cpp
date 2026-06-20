#include<bits/stdc++.h>
using namespace std;

int n,m,l,ans=0;
vector<int> v[100005];
int match[100005]={};
int book[100005]={};

bool dfs(int x){
	for(int y:v[x]){
		if(!book[y]){
			book[y]=1;
			if(!match[y] || dfs(match[y])){
				match[y]=x;
				return 1;
			}
		}
	}
	return 0;
}

int main(){
	cin>>n>>m>>l;
	for(int i=1;i<=l;i++){
		int a,b;
		cin>>a>>b;
		v[a].push_back(b);
	}
	for(int i=1;i<=n;i++){
		memset(book,0,sizeof(book));
		if(dfs(i)) ans++;
	}
	cout<<(n+m)-ans;
	return 0;
}
