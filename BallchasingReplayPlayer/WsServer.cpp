#include "pch.h"
#include "WsServer.h"

void WsServer::InitAsio()
{
	try
	{
		ws_server_.init_asio();
	}
	catch (websocketpp::exception const& e)
	{
		if (e.code().value() == 9)
		{
			//LOG("ASIO probably already initialized");
		}else
		{
			throw;
		}
	}
}

void WsServer::StartWebSocketServer(const uint16_t port)
{
	try
	{
		std::lock_guard lock(server_running_mutex_);
		port_ = port;

		InitAsio();
		ws_server_.set_message_handler([this](auto handle, auto message)
		{
			OnMessage(std::move(handle), std::move(message));
		});
		ws_server_.set_open_handler([this](auto handle) { OnClientConnected(std::move(handle)); });
		ws_server_.set_close_handler([this](auto handle) { OnClientDisconnected(std::move(handle)); });

		ws_server_.reset();
		ws_server_.listen(port_);
		ws_server_.start_accept();
		LOG("Local websocket server: Listening on {}.", port_);
		ws_server_.run();
		LOG("Local websocket server: Stopping");
	}
	catch (websocketpp::exception const& e)
	{
		LOG("Local websocket server: failed to init.");
		LOG(e.code().message());
		LOG(std::to_string(e.code().value()));
		LOG(e.what());
	}
	catch (std::exception& e)
	{
		LOG("{}", e.what());
	}
}

void WsServer::OnMessage(const websocketpp::connection_hdl& hdl, const MessagePtr& message)
{
	for(auto& [key, message_handler_cb]: message_handlers_)
	{
		message_handler_cb(&ws_server_, hdl, message);
	}
}

void WsServer::OnClientConnected(websocketpp::connection_hdl hdl)
{
	LOG("Local websocket server: Client connected");
	ws_connections_.insert(hdl);
}

void WsServer::OnClientDisconnected(websocketpp::connection_hdl hdl)
{
	LOG("Local websocket server: Client disonnected");
	ws_connections_.erase(hdl);
}

void WsServer::StartWebSocketServerAsync(const uint16_t port)
{
	LOG("Local websocket server: Startin server thread");
	server_thread_ = std::thread{[this, port] { StartWebSocketServer(port); }};
}

void WsServer::ShutdownWebSocketServer()
{
	if (ws_server_.is_listening())
	{
		LOG("Local websocket server: Not accepting new connections");
		ws_server_.stop_listening();
		for (const auto& hdl : ws_connections_)
		{
			auto con = ws_server_.get_con_from_hdl(hdl);
			LOG("Local websocket server: Closing a client connection");
			ws_server_.close(hdl, websocketpp::close::status::going_away, "Server stopping");
		}
		LOG("Local websocket server: Waiting..");
		std::lock_guard lock(server_running_mutex_);
		LOG("Local websocket server: Joining?");
		server_thread_.join();
		LOG("Local websocket server: Joined");
	}
}

void WsServer::AddMessageHandler(const MessageHandlerKey& key, MessageHandler message_handler)
{
	message_handlers_.insert_or_assign(key, std::move(message_handler));
}

void WsServer::Send(websocketpp::connection_hdl const& hdl, std::string const& msg)
{
	ws_server_.send(hdl, msg, websocketpp::frame::opcode::text);
}



WsServer::~WsServer()
{
	ShutdownWebSocketServer();
}
