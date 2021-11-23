/*
 * VncConnection.h - declaration of VncConnection class
 *
 * Copyright (c) 2008-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * code partly taken from KRDC / vncclientthread.h:
 * Copyright (C) 2007-2008 Urs Wolfer <uwolfer @ kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <rfb/rfbproto.h>

#include <QElapsedTimer>
#include <QImage>
#include <QMutex>
#include <QQueue>
#include <QReadWriteLock>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>

#include "SocketDevice.h"
#include "VeyonCore.h"
#include "VncConnectionConfiguration.h"

using rfbClient = struct _rfbClient;

class QSslSocket;
class VncEvent;

class VEYON_CORE_EXPORT VncConnection : public QThread
{
	Q_OBJECT
public:
	enum class Quality
	{
		Thumbnail,
		Screenshot,
		RemoteControl,
		Default
	} ;

	enum class FramebufferState
	{
		Invalid,
		Initialized,
		Valid
	} ;

	enum class State
	{
		None,
		Disconnected,
		Connecting,
		HostOffline,
		ServerNotRunning,
		AuthenticationFailed,
		ConnectionFailed,
		Connected
	} ;
	Q_ENUM(State)

	explicit VncConnection( QObject *parent = nullptr );

	static void initLogging( bool debug );

	QImage image();

	void restart();
	void stop();
	void stopAndDeleteLater();

	void setHost( const QString& host );
	void setPort( int port );

	State state() const
	{
		return m_state;
	}

	bool isConnected() const
	{
		return state() == State::Connected && isRunning();
	}

	const QString& host() const
	{
		return m_host;
	}

	void setQuality( Quality quality )
	{
		m_quality = quality ;
	}

	void setUseRemoteCursor( bool enabled );

	void setServerReachable();

	void enqueueEvent( VncEvent* event, bool wake );
	bool isEventQueueEmpty();

	/** \brief Returns whether framebuffer data is valid, i.e. at least one full FB update received */
	bool hasValidFramebuffer() const
	{
		return m_framebufferState == FramebufferState::Valid;
	}

	void setScaledSize( QSize s );

	QImage scaledFramebuffer();

	void setFramebufferUpdateInterval( int interval );

	void setSkipHostPing( bool on )
	{
		setControlFlag( ControlFlag::SkipHostPing, on );
	}

	void rescaleFramebuffer();

	static constexpr int VncConnectionTag = 0x590123;

	static void* clientData( rfbClient* client, int tag );
	void setClientData( int tag, void* data );

	static qint64 libvncClientDispatcher( char * buffer, const qint64 bytes,
										  SocketDevice::SocketOperation operation, void * user );

	void mouseEvent( int x, int y, uint buttonMask );
	void keyEvent( unsigned int key, bool pressed );
	void clientCut( const QString& text );

Q_SIGNALS:
	void connectionPrepared();
	void imageUpdated( int x, int y, int w, int h );
	void framebufferUpdateComplete();
	void framebufferSizeChanged( int w, int h );
	void cursorPosChanged( int x, int y );
	void cursorShapeUpdated( const QPixmap& cursorShape, int xh, int yh );
	void gotCut( const QString& text );
	void stateChanged();

protected:
	void run() override;

private:
	// RFB parameters
	using RfbPixel = uint32_t;
	static constexpr int RfbBitsPerSample = 8;
	static constexpr int RfbSamplesPerPixel = 3;
	static constexpr int RfbBytesPerPixel = sizeof(RfbPixel);

	enum class ControlFlag {
		ScaledFramebufferNeedsUpdate = 0x01,
		ServerReachable = 0x02,
		TerminateThread = 0x04,
		RestartConnection = 0x08,
		DeleteAfterFinished = 0x10,
		SkipHostPing = 0x20,
		RequiresManualUpdateRateControl = 0x40
	};

	~VncConnection() override;

	void establishConnection();
	void handleConnection();
	void closeConnection();

	void setState( State state );

	void setControlFlag( ControlFlag flag, bool on );
	bool isControlFlagSet( ControlFlag flag );

	rfbBool initFrameBuffer( rfbClient* client );
	void finishFrameBufferUpdate();

	rfbBool updateCursorPosition( int x, int y );
	void updateCursorShape( rfbClient* client, int xh, int yh, int w, int h, int bpp );
	void updateClipboard( const char *text, int textlen );

	void sendEvents();

	void deleteLaterInMainThread();

	static void rfbClientLogDebug( const char* format, ... );
	static void rfbClientLogNone( const char* format, ... );
	static void framebufferCleanup( void* framebuffer );

	rfbSocket openTlsSocket( const char* hostname, int port );
	int readFromTlsSocket( char* buffer, unsigned int len );
	int writeToTlsSocket( const char* buffer, unsigned int len );
	void closeTlsSocket();

	// intervals and timeouts
	int m_threadTerminationTimeout{VncConnectionConfiguration::DefaultThreadTerminationTimeout};
	int m_connectTimeout{VncConnectionConfiguration::DefaultConnectTimeout};
	int m_readTimeout{VncConnectionConfiguration::DefaultReadTimeout};
	int m_connectionRetryInterval{VncConnectionConfiguration::DefaultConnectionRetryInterval};
	int m_messageWaitTimeout{VncConnectionConfiguration::DefaultMessageWaitTimeout};
	int m_fastFramebufferUpdateInterval{VncConnectionConfiguration::DefaultFastFramebufferUpdateInterval};
	int m_framebufferUpdateWatchdogTimeout{VncConnectionConfiguration::DefaultFramebufferUpdateWatchdogTimeout};
	int m_socketKeepaliveIdleTime{VncConnectionConfiguration::DefaultSocketKeepaliveIdleTime};
	int m_socketKeepaliveInterval{VncConnectionConfiguration::DefaultSocketKeepaliveInterval};
	int m_socketKeepaliveCount{VncConnectionConfiguration::DefaultSocketKeepaliveCount};

	// states and flags
	std::atomic<State> m_state{State::Disconnected};
	std::atomic<FramebufferState> m_framebufferState{FramebufferState::Invalid};
	QAtomicInteger<uint> m_controlFlags{};

	QSslSocket* m_sslSocket{nullptr};
	const bool m_verifyServerCertificate{true};

	// connection parameters and data
	rfbClient* m_client{nullptr};
	Quality m_quality{Quality::Default};
	QString m_host{};
	int m_port{-1};
	int m_defaultPort{-1};
	bool m_useRemoteCursor{false};

	// thread and timing control
	QMutex m_globalMutex{};
	QMutex m_eventQueueMutex{};
	QWaitCondition m_updateIntervalSleeper{};
	QAtomicInt m_framebufferUpdateInterval{0};
	QElapsedTimer m_framebufferUpdateWatchdog{};

	// queue for RFB and custom events
	QQueue<VncEvent *> m_eventQueue{};

	// framebuffer data and thread synchronization objects
	QImage m_image{};
	QImage m_scaledFramebuffer{};
	QSize m_scaledSize{};
	QReadWriteLock m_imgLock{};

} ;
