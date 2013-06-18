/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LikewiseNegotiateAuthenticator.java
 *
 * Abstract:
 *
 *        Likewise Authentication for Tomcat
 *
 *        Likewise SPNEGO Authenticator Valve
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 * 
 */

package com.likewise.auth.valve.spnego;

import java.io.IOException;
import java.security.Principal;
import java.util.List;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;

import org.apache.catalina.authenticator.AuthenticatorBase;
import org.apache.catalina.connector.Request;
import org.apache.catalina.connector.Response;
import org.apache.catalina.deploy.LoginConfig;
import org.apache.catalina.realm.GenericPrincipal;
import org.apache.commons.codec.binary.Base64;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.likewise.auth.LikewiseUser;
import com.likewise.interop.AuthenticationAdapter;
import com.likewise.interop.GSSContext;
import com.likewise.interop.GSSResult;

/**
 * Likewise Authentication Valve for the Apache Tomcat servlet container.
 * <p>
 * This authentication valve provides SPNEGO authentication support using the
 * Likewise authentication stack as per 
 * RFC4559 {@link http://tools.ietf.org/html/rfc4559}.
 * </p>
 */
public class LikewiseNegotiateAuthenticator extends AuthenticatorBase
{
	private static final String AUTH_METHOD_NEGOTIATE = "Negotiate ";
	private static final String AUTH_METHOD_NTLM      = "NTLM ";
	private static final String GSS_CONTEXT           = "likewise-gss-context";
	private static final Log _logger;
	private static final byte[] NTLMSSP_SIGNATURE = 
							{0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00};
	
	static
	{
		_logger = LogFactory.getLog(LikewiseNegotiateAuthenticator.class);
	}
	
	/**
	 * Authenticates the current remote user trying to access a web page on the
	 * current system through the specified servlet request.
	 * 
	 * @param httpRequest  Incoming (HTTP) servlet request
	 * @param httpResponse Outgoing (HTTP) servlet response
	 * @param loginConfig  Configuration parameters
	 * @return true if the user was successfully authenticated.
	 */
	@Override
	protected
	boolean
	authenticate(
		Request     httpRequest,
		Response    httpResponse,
		LoginConfig loginConfig
		) throws IOException
	{
		boolean authenticated = false;
		Principal principal = httpRequest.getUserPrincipal();
		String authorization = httpRequest.getHeader("Authorization");
		String authType;

		try
		{
			if (context == null || context.getRealm() == null)
			{
				sendError(
						httpResponse,
						HttpServletResponse.SC_SERVICE_UNAVAILABLE);
			}
			else if (principal != null &&
				     !requiresSpecialNTLMAuthorization(httpRequest))
			{
				authenticated = true;
			}
			else if (authorization != null)
			{
				Base64 base64Codec = new Base64(-1);
				
				authType = getAuthType(authorization);
				
				String authContent = authorization.substring(authType.length());
				
				byte[] authContentBytes = 
								base64Codec.decode(authContent.getBytes());
				
				HttpSession session = httpRequest.getSession(true);
				GSSContext gssContext = (GSSContext)session.getAttribute(GSS_CONTEXT);
				if (gssContext == null)
				{
					gssContext = new GSSContext();
					session.setAttribute(GSS_CONTEXT, gssContext);
				}
				
				GSSResult gssResult = gssContext.authenticate(authContentBytes);
				
				byte[] security_blob_out = gssResult.getSecurityBlob();
				
				if (security_blob_out != null)
				{
					String continueToken = 
							new String(base64Codec.encode(security_blob_out));
					
					httpResponse.addHeader(
									"WWW-Authenticate",
									authType + continueToken);
				}
				
				if (gssResult.isContinueNeeded())
				{
					httpResponse.setHeader("Connection", "keep-alive");
					httpResponse.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
					httpResponse.flushBuffer();
				}
				else
				{
					String clientPrincipal = gssContext.getClientPrincipal();
					LikewiseUser userPrincipal = AuthenticationAdapter.findUserByName(
															clientPrincipal);
					
					List<String> roles = gssContext.getRoles();
					
					principal = new GenericPrincipal(
											context.getRealm(),
											clientPrincipal,
											null,
											roles,
											userPrincipal);
					
					register(
							httpRequest,
							httpResponse,
							principal,
							authType.trim(),
							principal.getName(),
							null);
					
					authenticated = true;
				}
			}
			else
			{
				HttpSession session = httpRequest.getSession(false);

				if (session != null)
				{
					session.removeAttribute(GSS_CONTEXT);
				}
				
				sendUnauthorizedResponse(httpResponse);
			}
		}
		catch(Exception e)
		{
			_logger.error(e.getMessage());
			sendUnauthorizedResponse(httpResponse);
		}
		
		return authenticated;
	}
	
	private
	boolean
	requiresSpecialNTLMAuthorization(HttpServletRequest httpRequest)
	{
		boolean result = false;
		String authorization = httpRequest.getHeader("Authorization");
		String httpMethod = httpRequest.getMethod();
		
		if ((httpMethod.equals("POST") || httpMethod.equals("PUT")) &&
		    (httpRequest.getContentLength() == 0) &&
			(authorization != null))
		{
			Base64 base64Codec = new Base64(-1);
			
			String authType = getAuthType(authorization);
			
			String authContent = authorization.substring(authType.length());
			
			byte[] authContentBytes = 
						base64Codec.decode(authContent.getBytes());
			
			if (authContentBytes.length > NTLMSSP_SIGNATURE.length)
			{
				boolean signatureMatches = true;
				
				for (int i = 0; i < NTLMSSP_SIGNATURE.length; i++)
				{
					if (authContentBytes[i] != NTLMSSP_SIGNATURE[i])
					{
						signatureMatches = false;
						break;
					}
				}
				
				if (signatureMatches && 
					(authContentBytes[NTLMSSP_SIGNATURE.length] == 1))
				{
					result = true;
				}
			}
		}
		
		return result;
	}
	
	/**
	 * Determines the authentication type of the incoming http request based on
	 * the authorization header.
	 * 
	 * @param authorization Identifier from the HTTP authorization header.
	 * @return supported authentication type ("NTLM" or "Negotiate").
	 */
	private String getAuthType(String authorization)
	{
		if (authorization.startsWith(AUTH_METHOD_NEGOTIATE))
		{
			return AUTH_METHOD_NEGOTIATE;
		}
		else if (authorization.startsWith(AUTH_METHOD_NTLM))
		{
			return AUTH_METHOD_NTLM;
		}
		else
		{
			throw new RuntimeException("Unsupported authentication type [" + 
										authorization + 
										"]");
		}
	}
	
	/**
	 * Builds an unauthorized HTTP response message.
	 * 
	 * @param httpResponse Outgoing (HTTP) servlet response.
	 */
	private void sendUnauthorizedResponse(Response httpResponse)
	{
		try
		{
			httpResponse.addHeader("WWW-Authenticate", "Negotiate");
			httpResponse.addHeader("WWW-Authenticate", "NTLM");
			httpResponse.setHeader("Connection", "close");
			httpResponse.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
			httpResponse.flushBuffer();		
		} 
		catch (IOException e) 
		{
			throw new RuntimeException(e);
		}
	}
	
	/**
	 * Builds a HTTP error response.
	 * 
	 * @param httpResponse Outgoing (HTTP) servlet response
	 * @param status       HTTP error status
	 */
	private void sendError(Response httpResponse, int status)
	{
		try
		{
			httpResponse.sendError(status);
		}
		catch(IOException e)
		{
			throw new RuntimeException(e);
		}
	}
}
