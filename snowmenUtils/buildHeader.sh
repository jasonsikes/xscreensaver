#! /usr/bin/env sh

# This should be run while in snowmen_assets directory

DESTINATION="../hacks/glx/snowmen_textures.h"

echo "#ifndef SNOWMAN_TEXTURES_H" >$DESTINATION
echo "#define SNOWMAN_TEXTURES_H" >>$DESTINATION

xxd --include base.png  >>$DESTINATION
xxd --include head.png  >>$DESTINATION
xxd --include torso.png  >>$DESTINATION
xxd --include hillTexture.png  >>$DESTINATION
xxd --include iceTexture.png  >>$DESTINATION
xxd --include shoreTexture.png  >>$DESTINATION
xxd --include treeTexture.png  >>$DESTINATION

echo "#endif /* SNOWMAN_TEXTURES_H */" >>$DESTINATION
