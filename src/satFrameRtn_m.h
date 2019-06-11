//
// Generated file, do not edit! Created by nedtool 4.6 from satFrameRtn.msg.
//

#ifndef _SATFRAMERTN_M_H_
#define _SATFRAMERTN_M_H_

#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0406
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



/**
 * Class generated from <tt>satFrameRtn.msg:20</tt> by nedtool.
 * <pre>
 * //
 * // TODO generated message class
 * //
 * packet SatFrameRtn
 * {
 *     int srcAddress;
 *     int randomSeed;
 *     int slotIndexWithinFrame;
 *     double channelRatio;
 * }
 * </pre>
 */
class SatFrameRtn : public ::cPacket
{
  protected:
    int srcAddress_var;
    int randomSeed_var;
    int slotIndexWithinFrame_var;
    double channelRatio_var;

  private:
    void copy(const SatFrameRtn& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const SatFrameRtn&);

  public:
    SatFrameRtn(const char *name=NULL, int kind=0);
    SatFrameRtn(const SatFrameRtn& other);
    virtual ~SatFrameRtn();
    SatFrameRtn& operator=(const SatFrameRtn& other);
    virtual SatFrameRtn *dup() const {return new SatFrameRtn(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getSrcAddress() const;
    virtual void setSrcAddress(int srcAddress);
    virtual int getRandomSeed() const;
    virtual void setRandomSeed(int randomSeed);
    virtual int getSlotIndexWithinFrame() const;
    virtual void setSlotIndexWithinFrame(int slotIndexWithinFrame);
    virtual double getChannelRatio() const;
    virtual void setChannelRatio(double channelRatio);
};

inline void doPacking(cCommBuffer *b, SatFrameRtn& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, SatFrameRtn& obj) {obj.parsimUnpack(b);}


#endif // ifndef _SATFRAMERTN_M_H_

