#pragma once

namespace pyr
{

enum class BlendProfile
{
  BLEND,
  NO_BLEND,
};

enum class DepthProfile
{
  TESTWRITE_DEPTH, // test&write
  TESTONLY_DEPTH, // test only
  NO_DEPTH, // no test no write
};

enum class RasterizerProfile
{
  CULLBACK_RASTERIZER,
  NOCULL_RASTERIZER,
};

class RenderProfiles
{
public:
  static void pushBlendProfile(BlendProfile profile);
  static void popBlendProfile();
  
  static void pushDepthProfile(DepthProfile profile);
  static void popDepthProfile();

  static void pushRasterProfile(RasterizerProfile profile);
  static void popRasterProfile();

public:

  static void initProfiles();
  static void disposeProfiles();

private:
  static void setActiveBlendProfile(BlendProfile profile);
  static void setActiveDepthProfile(DepthProfile profile);
  static void setActiveRasterProfile(RasterizerProfile profile);
};

}