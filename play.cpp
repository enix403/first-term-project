#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>

using namespace std;

struct A
{
    string hl;
    int x;
    float y;
    bool z;
};

template <size_t N>
void show_list(A (&as)[N])
{
    for (int i = 0; i < N; ++i)
    {
        cout << i << " -> ";
        const auto& a = as[i];
        cout << a.hl << "\t";
        cout << setw(4) << right << a.x;
        cout << setw(8) << right << a.y;
        cout << setw(3) << right << a.z;
        cout << endl;
    }
}


string make_string(int n)
{
    string res { "" };
    for (int i = 0; i < 24; ++i)
    {
        res += (char)('a' + i / n);
    }

    return res;
}

int main()
{
    const int N = 3;
    const int S = sizeof(A) * N;

    A a[N];

    a[0].hl = make_string(1);
    a[0].x = 72;
    a[0].y = 9.67;
    a[0].z = false;

    a[1].hl = make_string(2);
    a[1].x = 993;
    a[1].y = 3.1415;
    a[1].z = true;

    a[2].hl = make_string(3);
    a[2].x = -16;
    a[2].y = 5.5666;
    a[2].z = true;

    {
        ofstream fout("out.bin", ios::binary);
        fout.write(reinterpret_cast<char*>(&a), S);
        show_list(a);
    }

    cout << "------\n";

    {
        ifstream fin("out.bin", ios::binary);
        auto loc = reinterpret_cast<char*>(&a);

        for (int i = 0; i < N; ++i)
            a[i].~A();

        memset(loc, 0, S); /* Clear array memory just to be sure */

        fin.read(loc, S);
        show_list(a);
    }

    return 0;
}