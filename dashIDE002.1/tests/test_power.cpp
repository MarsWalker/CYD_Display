/**
 * @file test_power.cpp
 *
 * Exercise the real fixed-capacity power model and rendering template.
 * TestDisplay records primitive drawing calls and can export an optional SVG
 * when a filename is passed; neither output path accesses display hardware.
 */

#include "../power_history.h"
#include "../power_plot.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <array>
#include <string>
#include <cstring>
#include <fstream>

struct TestDisplay {
    std::vector<std::array<int,5>> lines;
    std::vector<std::array<int,5>> fills;
    std::ofstream svg;
    int cursorX=0,cursorY=0;
    unsigned color=0;
    explicit TestDisplay(const char* path=nullptr) {
        if(path){svg.open(path);svg<<"<svg xmlns='http://www.w3.org/2000/svg' width='320' height='240' viewBox='0 0 320 240'><rect width='320' height='240' fill='black'/>";}
    }
    ~TestDisplay(){if(svg.is_open())svg<<"</svg>";}
    int width(){return 320;}int height(){return 240;}
    std::string rgb(unsigned c){char b[8];snprintf(b,sizeof(b),"#%02x%02x%02x",((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31);return b;}
    void fillRect(int x,int y,int w,int h,unsigned c){fills.push_back({x,y,w,h,int(c)});if(svg.is_open())svg<<"<rect x='"<<x<<"' y='"<<y<<"' width='"<<w<<"' height='"<<h<<"' fill='"<<rgb(c)<<"'/>";}
    void drawRect(int x,int y,int w,int h,unsigned c){if(svg.is_open())svg<<"<rect x='"<<x<<"' y='"<<y<<"' width='"<<w-1<<"' height='"<<h-1<<"' fill='none' stroke='"<<rgb(c)<<"'/>";}
    void drawLine(int x,int y,int x1,int y1,unsigned c){lines.push_back({x,y,x1,y1,int(c)});if(svg.is_open())svg<<"<line x1='"<<x<<"' y1='"<<y<<"' x2='"<<x1<<"' y2='"<<y1<<"' stroke='"<<rgb(c)<<"'/>";}
    void setTextFont(int){}void setTextColor(unsigned c,unsigned){color=c;}
    void setCursor(int x,int y){cursorX=x;cursorY=y;}
    int textWidth(const char* s){return strlen(s)*6;}
    void print(const char* s){if(svg.is_open())svg<<"<text x='"<<cursorX<<"' y='"<<cursorY+8<<"' fill='"<<rgb(color)<<"' font-family='monospace' font-size='10'>"<<s<<"</text>";}
};

int main(int argc,char** argv){
 PowerHistory h;
 assert(h.maximum()==100&&!h.valid);
 assert(!h.record(0,"unknown")&&!h.record(0,"unavailable")&&!h.record(0,"nan")&&!h.record(0,"inf")&&!h.record(0,"12watts")&&!h.record(0,""));
 assert(h.record(0," -0.8 ",true));assert(h.latest==-800&&h.maximum()==840);
 h.record(2000,"0");assert(h.maximum()==840);
 h.prune(300001);assert(h.count==1&&h.maximum()==5&&h.latest==0);
 h.prune(302001);assert(h.count==0&&h.maximum()==100&&h.valid);
 h=PowerHistory{};
 for(unsigned i=0;i<1000;++i)h.record(i*2000,"5");
 assert(h.count==151&&h.at(0).time==1698000&&h.at(150).time==1998000);
 h=PowerHistory{};h.record(UINT32_MAX-999,"20");h.record(1000,"-30");h.prune(1000);assert(h.count==2);h.prune(301000);assert(h.count==1);
 h=PowerHistory{};h.record(0,"100");h.record(2000,"-100");
 std::vector<std::array<int,5>> split;
 PowerSegments(h,2000,0,0,300,100,h.maximum(),[&](int x,int y,int x1,int y1,bool n){split.push_back({x,y,x1,y1,n});});
 assert(split.size()==2&&split[0][4]==0&&split[1][4]==1);
 assert(split[0][2]==split[1][0]&&split[0][3]==100&&split[1][1]==100);
 h=PowerHistory{};
 const char* values[]={"-800","-400","0","700","1200","-300","500","0","-900","200"};
 for(unsigned i=0;i<10;++i)h.record(30000*(i+1),values[i]);
 assert(h.maximum()==1260);
 TestDisplay d(argc>1?argv[1]:nullptr);RenderPowerGraph(d,&h,300000,10,100,178,90);
 assert(d.fills.front()[4]==0x0861);
 TestDisplay custom;
 RenderPowerGraph(custom,&h,300000,10,100,178,90,PowerGraphColors{0,0xf800,0x001f});
 assert(custom.fills.front()[4]==0);
 assert(custom.lines.size()==d.lines.size());
 for(size_t i=0;i<d.lines.size();++i){
     for(int coordinate=0;coordinate<4;++coordinate)assert(custom.lines[i][coordinate]==d.lines[i][coordinate]);
     int expected=d.lines[i][4]==0xffe0?0xf800:(d.lines[i][4]==0x07e0?0x001f:d.lines[i][4]);
     assert(custom.lines[i][4]==expected);
 }
 d.setTextColor(0xffe0,0);d.setCursor(10,196);d.print("Casa:200W");
 printf("{\"maximum\":%g,\"segments\":[",h.maximum());
 bool first=true;
 for(auto l:d.lines)if(l[4]==0xffe0||l[4]==0x07e0){if(!first)printf(",");first=false;printf("[%d,%d,%d,%d,%d]",l[0],l[1],l[2],l[3],l[4]);assert(l[0]>=11&&l[2]<=186&&l[1]>=101&&l[3]<=188);}
 puts("]}");
 // Shrink the scale when the old peak leaves the actual five-minute window.
 h.prune(450001);assert(h.maximum()==945);
 TestDisplay clipped;RenderPowerGraph(clipped,&h,450001,310,235,178,90);assert(clipped.lines.empty());
}
