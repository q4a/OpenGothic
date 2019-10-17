#include "animationsolver.h"

#include "skeleton.h"
#include "pose.h"
#include "world/interactive.h"
#include "world/world.h"
#include "game/serialize.h"

#include "resources.h"

using namespace Tempest;

AnimationSolver::AnimationSolver() {
  }

void AnimationSolver::save(Serialize &fout) {
  fout.write(uint32_t(overlay.size()));
  for(auto& i:overlay){
    fout.write(i.skeleton->name(),i.time);
    }
  }

void AnimationSolver::load(Serialize &fin) {
  uint32_t sz=0;
  std::string s;

  fin.read(sz);
  overlay.resize(sz);
  for(auto& i:overlay){
    fin.read(s,i.time);
    i.skeleton = Resources::loadSkeleton(s);
    }

  sz=0;
  for(size_t i=0;i<overlay.size();++i){
    overlay[sz]=overlay[i];
    if(overlay[i].skeleton!=nullptr)
      ++sz;
    }
  overlay.resize(sz);
  }

void AnimationSolver::setSkeleton(const Skeleton *sk) {
  baseSk = sk;
  }

void AnimationSolver::addOverlay(const Skeleton* sk,uint64_t time) {
  if(sk==nullptr)
    return;
  Overlay ov = {sk,time};
  overlay.push_back(ov);
  }

void AnimationSolver::delOverlay(const char *sk) {
  if(overlay.size()==0)
    return;
  auto skelet = Resources::loadSkeleton(sk);
  delOverlay(skelet);
  }

void AnimationSolver::delOverlay(const Skeleton *sk) {
  for(size_t i=0;i<overlay.size();++i)
    if(overlay[i].skeleton==sk){
      overlay.erase(overlay.begin()+int(i));
      return;
      }
  }

void AnimationSolver::update(uint64_t tickCount) {
  for(size_t i=0;i<overlay.size();){
    auto& ov = overlay[i];
    if(ov.time!=0 && ov.time<tickCount)
      overlay.erase(overlay.begin()+int(i)); else
      ++i;
    }
  }

const Animation::Sequence* AnimationSolver::solveAnim(AnimationSolver::Anim a, WeaponState st, WalkBit wlkMode, const Pose& pose) const {
  // Atack
  if(st==WeaponState::Fist) {
    if(a==Anim::Atack && pose.isInAnim("S_FISTRUNL")) {
      if(auto s=solveFrm("T_FISTATTACKMOVE"))
        return s;
      }
    if(a==Anim::Atack)
      return solveFrm("S_FISTATTACK");
    if(a==Anim::AtackBlock)
      return solveFrm("T_FISTPARADE_0");
    }
  else if(st==WeaponState::W1H || st==WeaponState::W2H) {
    if(a==Anim::Atack && (pose.isInAnim("S_1HRUNL") || pose.isInAnim("S_2HRUNL")))
      return solveFrm("T_%sATTACKMOVE",st);
    if(a==Anim::AtackL)
      return solveFrm("T_%sATTACKL",st);
    if(a==Anim::AtackR)
      return solveFrm("T_%sATTACKR",st);
    if(a==Anim::Atack || a==Anim::AtackL || a==Anim::AtackR)
      return solveFrm("S_%sATTACK",st); // TODO: proper atack  window
    if(a==Anim::AtackBlock) {
      const Animation::Sequence* s=nullptr;
      switch(std::rand()%3){
        case 0: s = solveFrm("T_%sPARADE_0",   st); break;
        case 1: s = solveFrm("T_%sPARADE_0_A2",st); break;
        case 2: s = solveFrm("T_%sPARADE_0_A3",st); break;
        }
      if(s==nullptr)
        s = solveFrm("T_%sPARADE_0",st);
      return s;
      }
    if(a==Anim::AtackFinish)
      return solveFrm("T_%sSFINISH",st);
    }
  else if(st==WeaponState::Bow || st==WeaponState::CBow) {
    // S_BOWAIM -> S_BOWSHOOT+T_BOWRELOAD -> S_BOWAIM
    if(a==Anim::AimBow) {
      if(pose.isInAnim("S_BOWRUN") || pose.isInAnim("S_CBOWRUN"))
        return solveFrm("T_%sRUN_2_%sAIM",st); else
        return solveFrm("S_%sSHOOT",st);
      }
    if(a==Anim::Atack)
      return solveFrm("T_%sRELOAD",st);
    if(a==Anim::Idle) {
      if(pose.isInAnim("S_BOWSHOOT") || pose.isInAnim("S_CBOWSHOOT"))
        return solveFrm("T_%sAIM_2_%sRUN",st);
      }
    }
  else if(st==WeaponState::Mage) {
    // Magic-cast
    if(Anim::MagFirst<=a && a<=Anim::MagLast) {
      if(pose.isInAnim("S_MAGRUN"))
        return solveMag("T_MAGRUN_2_%sSHOOT",a); else
        return solveMag("S_%sSHOOT",a);
      }
    for(uint8_t i=Anim::MagFirst;i<=Anim::MagLast;++i) {
      auto sq = solveMag("S_%sSHOOT",Anim(i));
      if(pose.isInAnim(sq))
        return solveMag("T_%sSHOOT_2_STAND",Anim(i));
      }
    }
  if(a==Anim::MagNoMana)
    return solveFrm("T_CASTFAIL");
  // Move
  if(a==Idle) {
    if(bool(wlkMode & WalkBit::WM_Swim))
      return solveFrm("S_SWIM");
    if(bool(wlkMode&WalkBit::WM_Walk))
      return solveFrm("S_%sWALK",st);
    return solveFrm("S_%sRUN",st);
    }
  if(a==Move) {
    if(bool(wlkMode & WalkBit::WM_Swim))
      return solveFrm("S_SWIMF",st);
    if(bool(wlkMode & WalkBit::WM_Walk))
      return solveFrm("S_%sWALKL",st);
    if(bool(wlkMode & WalkBit::WM_Water))
      return solveFrm("S_%sWALKWL",st);
    return solveFrm("S_%sRUNL",st);
    }
  if(a==MoveL) {
    if(bool(wlkMode & WalkBit::WM_Swim))
      return solveFrm("S_SWIM"); // ???
    if(bool(wlkMode & WalkBit::WM_Walk))
      return solveFrm("T_%sWALKWSTRAFEL",st);
    if(bool(wlkMode & WalkBit::WM_Water))
      return solveFrm("T_%sWALKWSTRAFEL",st);
    return solveFrm("T_%sRUNSTRAFEL",st);
    }
  if(a==MoveR) {
    if(bool(wlkMode & WalkBit::WM_Swim))
      return solveFrm("S_SWIM"); // ???
    if(bool(wlkMode & WalkBit::WM_Walk))
      return solveFrm("T_%sWALKWSTRAFER",st);
    if(bool(wlkMode & WalkBit::WM_Water))
      return solveFrm("T_%sWALKWSTRAFER",st);
    return solveFrm("T_%sRUNSTRAFER",st);
    }
  if(a==Anim::MoveBack) {
    if(bool(wlkMode & WalkBit::WM_Swim))
      return solveFrm("S_SWIMB");
    return solveFrm("T_%sJUMPB",st);
    }
  // Rotation
  if(a==RotL) {
    if(bool(wlkMode & WalkBit::WM_Swim))
      return solveFrm("T_SWIMTURNL");
    if(bool(wlkMode & WalkBit::WM_Walk))
      return solveFrm("T_%sWALKTURNL",st);
    if(bool(wlkMode & WalkBit::WM_Water))
      return solveFrm("T_%sWALKWTURNL",st);
    return solveFrm("T_%sRUNTURNL",st);
    }
  if(a==RotR) {
    if(bool(wlkMode & WalkBit::WM_Swim))
      return solveFrm("T_SWIMTURNR");
    if(bool(wlkMode & WalkBit::WM_Walk))
      return solveFrm("T_%sWALKTURNR",st);
    if(bool(wlkMode & WalkBit::WM_Water))
      return solveFrm("T_%sWALKWTURNR",st);
    return solveFrm("T_%sRUNTURNR",st);
    }
  // Jump regular
  if(a==Jump) {
    if(pose.isIdle())
      return solveFrm("T_STAND_2_JUMP");
    return solveFrm("S_JUMP");
    }
  if(a==JumpUpLow) {
    if(pose.isIdle())
      return solveFrm("T_STAND_2_JUMPUPLOW");
    return solveFrm("S_JUMPUPLOW");
    }
  if(a==JumpUpMid) {
    if(pose.isIdle())
      return solveFrm("T_STAND_2_JUMPUPMID");
    return solveFrm("S_JUMPUPMID");
    }
  if(a==JumpUp) {
    if(pose.isIdle())
      return solveFrm("T_STAND_2_JUMPUP");
    return solveFrm("S_JUMPUP");
    }

  if(a==Anim::Fall)
    return solveFrm("S_FALLDN");
  if(a==Anim::FallDeep)
    return solveFrm("S_FALL");
  if(a==Anim::SlideA)
    return solveFrm("S_SLIDE");
  if(a==Anim::SlideB)
    return solveFrm("S_SLIDEB");
  if(a==Anim::StumbleA)
    return solveFrm("T_STUMBLE");
  if(a==Anim::StumbleB)
    return solveFrm("T_STUMBLEB");
  if(a==Anim::DeadA) {
    if(pose.isInAnim("S_WOUNDED")  || pose.isInAnim("T_STAND_2_WOUNDED") ||
       pose.isInAnim("S_WOUNDEDB") || pose.isInAnim("T_STAND_2_WOUNDEDB"))
      return solveDead("T_WOUNDED_2_DEAD","T_WOUNDEDB_2_DEADB"); else
      return solveDead("T_DEAD", "T_DEADB");
    }
  if(a==Anim::DeadB) {
    if(pose.isInAnim("S_WOUNDED")  || pose.isInAnim("T_STAND_2_WOUNDED") ||
       pose.isInAnim("S_WOUNDEDB") || pose.isInAnim("T_STAND_2_WOUNDEDB"))
      return solveDead("T_WOUNDEDB_2_DEADB","T_WOUNDED_2_DEAD"); else
      return solveDead("T_DEADB","T_DEAD");
    }
  if(a==Anim::UnconsciousA)
    return solveFrm("S_WOUNDED");
  if(a==Anim::UnconsciousB)
    return solveFrm("S_WOUNDEDB");

  if(Anim::Dialog1<=a && a<=Anim::Dialog21){
    char buf[32]={};
    std::snprintf(buf,sizeof(buf),"T_DIALOGGESTURE_%02d",int(a-Anim::Dialog1+1));
    return solveFrm(buf);
    }
  return nullptr;
  }

const Animation::Sequence *AnimationSolver::solveAnim(WeaponState st, WeaponState cur, const Pose &pose) const {
  // Weapon draw/undraw
  if(st==cur)
    return nullptr;
  if(st==WeaponState::NoWeapon){
    if(pose.isInAnim("S_1HRUNL")   || pose.isInAnim("S_2HRUNL") ||
       pose.isInAnim("S_BOWRUNL")  || pose.isInAnim("S_CBOWRUNL") ||
       pose.isInAnim("S_FISTRUNL") || pose.isInAnim("S_MAGRUNL"))
      return solveFrm("T_%sMOVE_2_MOVE",cur);
    return solveFrm("T_%sRUN_2_%s",cur);
    }
  if(st==WeaponState::Fist ||
     st==WeaponState::W1H  || st==WeaponState::W2H ||
     st==WeaponState::Bow  || st==WeaponState::CBow ||
     st==WeaponState::Mage){
    if(pose.isInAnim("S_RUNL"))
      return solveFrm("T_MOVE_2_%sMOVE",st);
    return solveFrm("T_%s_2_%sRUN",st);
    }
  return nullptr;
  }

const Animation::Sequence *AnimationSolver::solveAnim(Interactive *inter, AnimationSolver::Anim a, const Pose &) const {
  if(inter==nullptr)
    return nullptr;
  if(a==AnimationSolver::InteractOut) {
    return inter->anim(*this,Interactive::Out);
    } else {
    return inter->anim(*this,Interactive::In);
    }
  }

const Animation::Sequence *AnimationSolver::solveFrm(const char *format, WeaponState st) const {
  static const char* weapon[] = {
    "",
    "FIST",
    "1H",
    "2H",
    "BOW",
    "CBOW",
    "MAG"
    };
  char name[128]={};
  std::snprintf(name,sizeof(name),format,weapon[int(st)],weapon[int(st)]);
  if(auto ret=solveFrm(name))
    return ret;
  std::snprintf(name,sizeof(name),format,"");
  if(auto ret=solveFrm(name))
    return ret;
  std::snprintf(name,sizeof(name),format,"FIST");
  return solveFrm(name);
  }

const Animation::Sequence *AnimationSolver::solveFrm(const char *name) const {
  if(name==nullptr || name[0]=='\0')
    return nullptr;
  for(size_t i=overlay.size();i>0;){
    --i;
    if(auto s = overlay[i].skeleton->sequence(name))
      return s;
    }
  return baseSk ? baseSk->sequence(name) : nullptr;
  }

const Animation::Sequence* AnimationSolver::solveMag(const char *format,Anim spell) const {
  static const char* mg[]={"FIB", "WND", "HEA", "PYR", "FEA", "TRF", "SUM", "MSD", "STM", "FRZ", "SLE", "WHI", "SCK", "FBT"};

  char name[128]={};
  std::snprintf(name,sizeof(name),format,mg[spell-Anim::MagFirst]);
  if(auto a=solveFrm(name))
    return a;
  std::snprintf(name,sizeof(name),format,mg[0]);
  return solveFrm(name);
  }

const Animation::Sequence *AnimationSolver::solveDead(const char *format1, const char *format2) const {
  if(auto a=solveFrm(format1))
    return a;
  return solveFrm(format2);
  }
