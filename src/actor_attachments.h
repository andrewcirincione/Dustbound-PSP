#ifndef DUST_ACTOR_ATTACHMENTS_H
#define DUST_ACTOR_ATTACHMENTS_H
/* Packed generated-art coordinates. These are garment landmarks, not guesses
   from the bounding-box centre. The painted neck and waist define the spine;
   the two socket points locate the arm attachments in that same transform. */
typedef struct {
    float neckX,neckY,waistX,waistY,nearX,nearY,farX,farY;
} TorsoFit;
static const TorsoFit torsoFits[16]={
    {24,4.5f,22,27,  11,14.5f,34,13}, /* trail duster */
    {25,5,23,31,     10,15,34,13}, /* ronin coat */
    {25,4,21,28,     10,14,36,13}, /* lamellar */
    {25,5,22,38,      8,16,33,14}, /* leather vest */
    {23,5,21,39,    8.5f,15.5f,34,14}, /* brigandine */
    {26,5,22,35,      8,15,35,14}, /* bone harness */
    {25,4,22,31,     10,14,36,13}, /* iron cuirass */
    {25,5,21,26,     10,14,33,13}, /* robes */
    {25,5,22,30,      9,15,36,14}, /* ash cloak */
    {26,5,22,35,      9,16,35,14}, /* gambeson */
    {25,5,21,28,     11,14,34,13}, /* plated duster */
    {26,5,21,37,      9,16,36,14}, /* reed jacket */
    {25,4,21,28,     11,14,36,13}, /* scale haori */
    {26,5,21,36,      9,16,36,14}, /* bandolier */
    {25,5,21,37,      9,16,35,14}, /* scavenger wraps */
    {25,4,21,31,     11,14,36,13}, /* captain armor */
};
static const TorsoFit bareTorsoFit={8.5f,3,9,26,3.5f,7,14,7};
static const float actorArmLength=5.4f,actorArmReach=10.6f,actorArmHang=9.2f;
#endif
