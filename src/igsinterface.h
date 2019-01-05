/*
* IGSInterface.h
*/

#ifndef IGSINTERFACE_H
#define IGSINTERFACE_H

#include <QString>


// maybe this should be elsewhere ...
typedef void (*callBackFP_t)(QString);

template <class T>  class callBackImpl
{
public:
	callBackImpl()
	{
		callbackFP = nullptr;
	}

	void registerCallback(T fp)
	{
		if (fp != nullptr) callbackFP = fp;
	};

protected:
	T callbackFP;
};


class IGSInterface : public callBackImpl<callBackFP_t>
{
public:
	IGSInterface() { }
	virtual ~IGSInterface()	{ }

	virtual bool isConnected()=0;
	virtual bool openConnection(const QString &host, unsigned port, const QString &user, const QString &pass)=0;
	virtual bool closeConnection()=0;
	virtual void sendTextToHost(QString, bool)=0;
	virtual void setTextCodec(QString)=0;
};
#endif
