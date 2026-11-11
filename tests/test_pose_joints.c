/* Check painted coverage at the joints, not merely coincident bone endpoints. */
#include "../src/render.c"
#include <assert.h>
static uint32_t canvas[256*240], upper[256*240];
static void blank(void){memset(canvas,0,sizeof(canvas));fb=canvas;pitch=256;clipLeft=clipTop=0;clipRight=256;clipBottom=240;}
static void painted_elbows(void){
    const int pairs[][2]={{72,75},{72,73},{74,75},{74,73},{76,77}};
    int cases=0;
    for(int scale=2;scale<=3;scale++)for(int type=0;type<5;type++)
    for(int side=0;side<2;side++)for(int direction=0;direction<32;direction++)for(int reach=0;reach<=16;reach++){
        float shoulder[2]={128,110},angle=direction*6.2831853f/32;
        float distance=reach<5?reach*.025f:(reach-4)*actorArmReach/12.f;
        float wrist[2]={128+cosf(angle)*distance*scale,110+sinf(angle)*distance*scale};
        ArmElbow e=arm_elbow(side,shoulder,wrist,scale,0,side?1:-1);
        assert(fabsf(hypotf(e.x-shoulder[0],e.y-shoulder[1])-actorArmLength*scale)<.001f);
        assert(fabsf(hypotf(wrist[0]-e.x,wrist[1]-e.y)-actorArmLength*scale)<.001f);
        blank();joint_sprite(pairs[type][0],shoulder[0],shoulder[1],e.x,e.y,2.5f*scale,0xd0a070,1,side);
        memcpy(upper,canvas,sizeof(upper));blank();
        joint_sprite(pairs[type][1],e.x,e.y,wrist[0],wrist[1],2.2f*scale,0xd0a070,1,side);
        int overlap=0;
        for(int y=(int)e.y-3*scale;y<e.y+3*scale;y++)for(int x=(int)e.x-3*scale;x<e.x+3*scale;x++)
            if((upper[y*256+x]>>24)>32&&(canvas[y*256+x]>>24)>32)overlap++;
        if(!overlap){fprintf(stderr,"Painted elbow gap: scale=%d type=%d side=%d direction=%d reach=%d\n",scale,type,side,direction,reach);assert(0);}
        cases++;
    }
    printf("PASS: %d painted elbow overlaps, including sleeves, bracers, bare and mechanical arms.\n",cases);
}
static void painted_wrists(void){
    int cases=0;
    for(int scale=2;scale<=3;scale++)for(int id=73;id<=77;id+=2)
    for(int side=0;side<2;side++)for(int direction=0;direction<32;direction++)for(int relaxed=0;relaxed<2;relaxed++){
        float angle=direction*6.2831853f/32,ux=cosf(angle)*scale,uy=sinf(angle)*scale;
        float hx=128,hy=110,overlap=relaxed?0:.65f;
        blank();joint_sprite(id,hx-ux*actorArmLength,hy-uy*actorArmLength,hx-ux*overlap,hy-uy*overlap,2.2f*scale,0xd0a070,1,side);
        memcpy(upper,canvas,sizeof(upper));blank();
        float start=relaxed?-.2f:-.8f,end=relaxed?1.4f:1.f;
        part(relaxed?79:78,hx+ux*start,hy+uy*start,hx+ux*end,hy+uy*end,
             (relaxed?2.f:2.1f)*scale,0xd0a070,1,0,side,0,1);
        int overlapPixels=0;
        for(int y=100;y<120;y++)for(int x=118;x<138;x++)
            if((upper[y*256+x]>>24)>32&&(canvas[y*256+x]>>24)>32)overlapPixels++;
        if(!overlapPixels){fprintf(stderr,"Painted wrist gap: scale=%d id=%d side=%d direction=%d relaxed=%d\n",scale,id,side,direction,relaxed);assert(0);}
        cases++;
    }
    printf("PASS: %d painted wrist overlaps for relaxed and gripping hands.\n",cases);
}
int main(void){game_new(73819,0);painted_elbows();painted_wrists();return 0;}
