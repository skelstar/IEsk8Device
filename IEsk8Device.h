#include <Arduino.h>

class IEsk8Device
{

public:
  typedef void (*callBack)();
  typedef void (*notifyCallBack)(const uint8_t *data, uint8_t data_len);

  virtual void connect() = 0;
  virtual void update();

  virtual void setOnConnectedEvent(callBack ptr_onConnectedEvent);
  virtual void setOnDisconnectedEvent(callBack ptr_onDisconnectedEvent);
  virtual void setOnNotifyEvent(notifyCallBack ptr_onNotifyEvent);
  virtual void setOnSentEvent(callBack ptr_onSentEvent);

// protected:
  
  callBack _onConnectedEvent;
  callBack _onDisconnectedEvent;
  notifyCallBack _onNotifyEvent;
  callBack _onSentEvent;
};

