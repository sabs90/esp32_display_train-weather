#ifndef __APP_H__
#define __APP_H__

class IApp {
 public:
  virtual bool fetchData();
  virtual void render();
};

#endif