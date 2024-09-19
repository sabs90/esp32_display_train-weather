#ifndef __APP_H__
#define __APP_H__

class IApp {
 public:
  virtual ~IApp() {} //what the heck is this
  //virtual bool fetchData();
  //virtual void render();
  virtual bool fetchData() = 0;
  virtual void render() = 0;
};

#endif