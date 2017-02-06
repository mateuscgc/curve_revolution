#include <bits/stdc++.h>

using namespace std;

#define X first
#define Y second
typedef pair<double, double> point2D;

const double delta = 1e-1;
list <point2D> points;


double sx(list<point2D>::iterator it) { return abs(prev(it)->X - it->X); }
double sy(list<point2D>::iterator it) { return abs(prev(it)->Y - it->Y); }
double sx(list<point2D>::iterator it, double x) { return abs(it->X - x); }
double sy(list<point2D>::iterator it, double y) { return abs(it->Y - y); }

double cross_product(list<point2D>::iterator a, list<point2D>::iterator b) {
    return sx(a)*sy(b) - sx(b)*sy(a);
}

double cross_product(list<point2D>::iterator it, point2D pt) {
    return sx(it)*sy(prev(it), pt.Y) - sx(prev(it), pt.X)*sy(it);  
}

bool in_interval(double a, double l1, double l2) {
    return (min(l1, l2) <= a && a <= max(l1, l2));
}

bool point_in_segment(list<point2D>::iterator it, point2D pt) {
    return (cross_product(it, pt) == 0 && in_interval(pt.X, prev(it)->X, it->X) && in_interval(pt.Y, prev(it)->Y, it->Y));
}

list<point2D>::iterator get_segment(point2D pt) {
    for(list<point2D>::iterator it = next(points.begin()); it != points.end(); it++)
        if(point_in_segment(it, pt))
            return it;
    return points.end();
}

list<point2D>::iterator add_point_before(point2D pt, list<point2D>::iterator it) {
    return points.insert(it, pt);
}

list<point2D>::iterator point_exists(point2D pt) {
    for(list<point2D>::iterator it = points.begin(); it != points.end(); it++)
        if(*it == pt)
            return it;
    return points.end();   
}

void remove_point(list<point2D>::iterator pt) {
    points.erase(pt);
}

void move_point(list<point2D>::iterator it, point2D pt) {
    *it = pt;
}

void dump_curve() {
    double t = 0;
    for(list<point2D>::iterator pt = points.begin(); pt != points.end(); pt++)
        cout << pt->X << " " << pt->Y << endl;
    cout << endl;
    while(t <= 1) {
        for(list<point2D>::iterator pt = next(points.begin()); next(pt) != points.end(); pt++) {
            cout << (1-t)*(1-t)*prev(pt)->X + 2*(1-t)*t*pt->X + t*t*next(pt)->X
                 << " " 
                 << (1-t)*(1-t)*prev(pt)->Y + 2*(1-t)*t*pt->Y + t*t*next(pt)->Y << endl;
        }
        t += delta;
    }
}


int main() {

    int n;
    cin >> n;

    double x, y;
    for(int i = 0; i < n; i++) {
        cin >> x >> y;
        points.push_back({x,y});
    }

    dump_curve();

    while(true) {
        char op;
        double x, y, xi, yi;
        list<point2D>::iterator it;

        cin >> op;
        switch(op) {
            case 'A':
                cin >> x >> y;
                it = get_segment({x,y});
                if(point_exists({x,y}) == points.end())
                    add_point_before({x,y}, it);
            break;

            case 'R':
                cin >> x >> y;
                if((it = point_exists({x, y})) != points.end())
                    remove_point(it);
            break;

            case 'M':
                cin >> xi >> yi >> x >> y;
                if((it = point_exists({xi, yi})) != points.end())
                    move_point(it, {x, y});
            break;

        }
        dump_curve();
    }
}