//-- includes -----
#include "ClientRequestManager.h"
#include "ClientNetworkManager.h"
#include "PSMoveProtocolInterface.h"
#include "PSMoveProtocol.pb.h"
#include <cassert>
#include <map>
#include <utility>

//-- definitions -----
struct RequestContext
{
    RequestPtr request;  // std::shared_ptr<PSMoveProtocol::Request>
    ClientPSMoveAPI::t_response_callback callback;
    void *callback_userdata;
};
typedef std::map<int, RequestContext> t_request_context_map;
typedef std::map<int, RequestContext>::iterator t_request_context_map_iterator;
typedef std::pair<int, RequestContext> t_id_request_context_pair;

class ClientRequestManagerImpl
{
public:
    ClientRequestManagerImpl()
        : m_pending_requests()
        , m_next_request_id(0)
    {
    }

    void send_request(RequestPtr request, ClientPSMoveAPI::t_response_callback callback, void *userdata)
    {
        RequestContext context;

        request->set_request_id(m_next_request_id);
        ++m_next_request_id;

        context.request= request;
        context.callback= callback;
        context.callback_userdata= userdata;

        // Add the request to the pending request map.
        // Requests should never be double registered.
        assert(m_pending_requests.find(request->request_id()) == m_pending_requests.end());
        m_pending_requests.insert(t_id_request_context_pair(request->request_id(), context));

        // Send the request off to the network manager to get sent to the server
        ClientNetworkManager::get_instance()->send_request(request);
    }

    void handle_request_canceled(RequestPtr request)
    {
        // Create a general canceled result
        ResponsePtr response(new PSMoveProtocol::Response);

        response->set_type(PSMoveProtocol::Response_ResponseType_GENERAL_RESULT);
        response->set_request_id(request->request_id());
        response->set_result_code(PSMoveProtocol::Response_ResultCode_RESULT_CANCELED);

        handle_response(response);
    }

    void handle_response(ResponsePtr response)
    {
        // Get the request awaiting completion
        t_request_context_map_iterator pending_request_entry= m_pending_requests.find(response->request_id());
        assert(pending_request_entry != m_pending_requests.end());

        // The context holds everything a handler needs to evaluate a response
        const RequestContext &context= pending_request_entry->second;

        // Notify the callback of the response
        if (context.callback != nullptr)
        {
            ClientPSMoveAPI::eClientPSMoveResultCode result;

            // Translate internal result codes into public facing result codes
            switch (response->result_code())
            {
            case PSMoveProtocol::Response_ResultCode_RESULT_OK:
                result= ClientPSMoveAPI::_clientPSMoveResultCode_ok;
                break;
            case PSMoveProtocol::Response_ResultCode_RESULT_ERROR:
                result= ClientPSMoveAPI::_clientPSMoveResultCode_error;
                break;
            case PSMoveProtocol::Response_ResultCode_RESULT_CANCELED:
                result= ClientPSMoveAPI::_clientPSMoveResultCode_canceled;
                break;
            default:
                assert(false && "Unknown response result code");
            }

            context.callback(
                result, 
                context.request->request_id(), 
                static_cast<ClientPSMoveAPI::t_response_handle>(response.get()), 
                context.callback_userdata);
        }

        // Remove the pending request from the map
        m_pending_requests.erase(pending_request_entry);
    }

private:
    t_request_context_map m_pending_requests;
    int m_next_request_id;
};

//-- public methods -----
ClientRequestManager::ClientRequestManager() 
{
    m_implementation_ptr = new ClientRequestManagerImpl();
}

ClientRequestManager::~ClientRequestManager()
{
    delete m_implementation_ptr;
}

void ClientRequestManager::send_request(
    RequestPtr request, ClientPSMoveAPI::t_response_callback callback, void *userdata)
{
    m_implementation_ptr->send_request(request, callback, userdata);
}

void ClientRequestManager::handle_request_canceled(RequestPtr request)
{
    m_implementation_ptr->handle_request_canceled(request);
}

void ClientRequestManager::handle_response(ResponsePtr response)
{
    m_implementation_ptr->handle_response(response);
}