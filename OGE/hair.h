#ifndef __HAIR_H_
#define __HAIR_H_

#include "cloth_sim.h"

class hair : public cloth_sim
{
public:
	virtual void init();
	
	// Need to overwrite for GR_TRIANGLES
	void draw();

private:
	void create_dread(unsigned long p_segment_count);
	void create_segment(unsigned long p_segment_number);
};

#endif // __HAIR_H_

