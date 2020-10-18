#include <bits/stdc++.h>


using namespace std;

int main()
{
    int n,t,c,t1,t2;
    cin >> n;
    cin >> t;
    int n1, n2;
    cin >> n1 >> n2 >> c >> t1 >> t2;
    int i;
    string str = "bgpv";
    cout << n << " "<< n1 << " "<< n2 << " " << c << " " << t1 << " " << t2 <<" " <<  t << "\n";
    for(i=0;i<n;i++)
    {
        cout << i << " " << str[rand()%4] << " " << rand()%t << "\n";
    }
    return 0;
}