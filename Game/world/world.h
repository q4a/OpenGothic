#pragma once

#include <Tempest/VertexBuffer>
#include <Tempest/IndexBuffer>
#include <Tempest/Matrix4x4>
#include <string>

#include <daedalus/DaedalusVM.h>
#include <zenload/zTypes.h>
#include <zenload/zTypes.h>

#include "graphics/worldview.h"
#include "graphics/staticobjects.h"
#include "physics/dynamicworld.h"
#include "worldscript.h"
#include "item.h"
#include "npc.h"
#include "interactive.h"
#include "resources.h"
#include "trigger.h"
#include "worldobjects.h"

class Gothic;
class RendererStorage;
class Focus;

class World final {
  public:
    World()=delete;
    World(const World&)=delete;
    World(Gothic &gothic,const RendererStorage& storage, std::string file);

    bool isEmpty() const { return wname.empty(); }
    const std::string& name() const { return wname; }

    const ZenLoad::zCWaypointData* findPoint(const std::string& s) const { return findPoint(s.c_str()); }
    const ZenLoad::zCWaypointData* findPoint(const char* name) const;

    WorldView*    view()   const { return wview.get();    }
    DynamicWorld* physic() const { return wdynamic.get(); }
    WorldScript*  script() const { return vm.get();       }

    StaticObjects::Mesh getView(const std::string& visual);
    StaticObjects::Mesh getView(const std::string& visual, int32_t headTex, int32_t teetTex, int32_t bodyColor);
    DynamicWorld::Item  getPhysic(const std::string& visual);

    void     updateAnimation();

    Npc*     player() const { return npcPlayer; }
    void     tick(uint64_t dt);
    uint64_t tickCount() const;
    void     setDayTime(int32_t h,int32_t min);
    gtime    time() const;

    Daedalus::PARSymbol& getSymbol(const char* s) const;
    size_t               getSymbolIndex(const char* s) const;

    Focus findFocus(const Npc& pl,const Tempest::Matrix4x4 &mvp, int w, int h);
    Focus findFocus(const Tempest::Matrix4x4 &mvp, int w, int h);

    const Trigger* findTrigger(const std::string& s) const { return findTrigger(s.c_str()); }
    const Trigger* findTrigger(const char* name) const;

    void marchInteractives(Tempest::Painter& p, const Tempest::Matrix4x4 &mvp, int w, int h) const;

    auto  updateDialog(const WorldScript::DlgChoise &dlg) -> std::vector<WorldScript::DlgChoise>;
    void  exec(const WorldScript::DlgChoise& dlg, Npc& player,Npc& hnpc);

    void  aiProcessInfos(Npc &player, Npc& npc);
    void  aiOutput(const char* msg);
    void  aiCloseDialog();
    bool  aiIsDlgFinished();

    void  printScreen(const char* msg, int x, int y, int time,const Tempest::Font &font);
    void  print      (const char* msg);

    void  onInserNpc (Daedalus::GameState::NpcHandle handle, const std::string &s);
    Item* addItem    (size_t itemInstance, const char *at);

  private:
    std::string                           wname;
    Gothic&                               gothic;
    ZenLoad::zCWayNetData                 wayNet;
    std::vector<ZenLoad::zCWaypointData>  freePoints, startPoints;
    std::vector<ZenLoad::zCWaypointData*> indexPoints;

    Npc*                                  npcPlayer=nullptr;

    std::unique_ptr<DynamicWorld>         wdynamic;
    std::unique_ptr<WorldView>            wview;
    WorldObjects                          wobj;
    std::unique_ptr<WorldScript>          vm;

    void         adjustWaypoints(std::vector<ZenLoad::zCWaypointData>& wp);
    void         loadVob(const ZenLoad::zCVobData &vob);
    void         addStatic(const ZenLoad::zCVobData &vob);
    void         addInteractive(const ZenLoad::zCVobData &vob);
    void         addItem(const ZenLoad::zCVobData &vob);

    void         initScripts(bool firstTime);
    int32_t      runFunction(const std::string &fname);
  };