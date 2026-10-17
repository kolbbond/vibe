// test hitbox overlap functions

#include <cassert>

#include "smg/GuiBase.hh"

#include "vibe/graphics/Hitbox.hxx"

int main(int argc, char** argv) {

	// create a guibase to ensure GL context
    // @hey, must be a better way to have a GL context
	smg::GuiBase gb({argc, argv});

	// create hitboxes (unit cube
	arma::Mat<fltp> Rn1(3, 8);
	Rn1.row(0) = arma::Row<fltp>{0, 1, 1, 0, 0, 1, 1, 0}; // x
	Rn1.row(1) = arma::Row<fltp>{0, 0, 1, 1, 0, 0, 1, 1};
	Rn1.row(2) = arma::Row<fltp>{0, 0, 0, 0, 1, 1, 1, 1};

	arma::Mat<fltp> Rn2 = Rn1;
	arma::Mat<fltp> Rn3 = Rn1;
	Rn2.row(2) += 0.5; // this will overlap
	Rn3.row(2) += 2; // this won't

    printf("creating hitboxes\n");
	vibe::ShHitboxPr box1 = vibe::Hitbox::create();
	vibe::ShHitboxPr box2 = vibe::Hitbox::create();
	vibe::ShHitboxPr box3 = vibe::Hitbox::create();

    printf("set vertices\n");
	box1->set_vertices(Rn1);
	box2->set_vertices(Rn2);
	box3->set_vertices(Rn3);

    printf("calculate hitbox bounds\n");
	box1->calc_bounds();
	box2->calc_bounds();
	box3->calc_bounds();

	// check if overlap
    printf("check overlap\n");
	assert(vibe::Hitbox::overlap(box1, box2));
	assert(!vibe::Hitbox::overlap(box1, box3));

    printf("done!\n");
}
