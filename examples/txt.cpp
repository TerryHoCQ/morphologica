/*
 * TxtVisual example
  */
#include <morph/Visual.h>
#include <morph/TxtVisual.h>
#include <morph/vec.h>

int main()
{
    morph::Visual v (1024, 768, "TxtVisual example");

    auto tv = std::make_unique<morph::TxtVisual<>> ("This is text\nand it's a VisualModel\n"
                                                    "It rotates and translates with the scene.\n"
                                                    "You can use newline characters [here]-->\n"
                                                    "in the source and these are reflected in the output.",
                                                    morph::vec<float>{1.0f, 0.0f, 0.5f}, morph::TextFeatures (0.1f));
    v.bindmodel (tv);
    tv->finalize();
    v.addVisualModel (tv);

    v.keepOpen();

    return 0;
}
