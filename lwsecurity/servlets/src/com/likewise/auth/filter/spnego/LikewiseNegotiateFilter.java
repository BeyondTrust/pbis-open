/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LikewiseNegotiateFilter.java
 *
 * Abstract:
 *
 *        Likewise Authentication
 *        
 *        Servlet Filter performing SPNEGO
 *
 * Authors:
 * Sriram Nambakam (snambakam@likewise.com)
 * Vyacheslav Minyailov (slavam@likewise.com)
 *
 */

package com.likewise.auth.filter.spnego;

import java.io.IOException;
import java.security.Principal;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.StringTokenizer;
import javax.security.auth.Subject;
import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;
import org.apache.commons.codec.binary.Base64;
import org.apache.commons.net.util.SubnetUtils;
import com.likewise.auth.LikewiseGroup;
import com.likewise.auth.LikewiseUser;
import com.likewise.interop.AuthenticationAdapter;
import com.likewise.interop.GSSContext;
import com.likewise.interop.GSSResult;

/**
 * Servlet authentication filter that provides SPNEGO authentication support as
 * per RFC4559 {link http://tools.ietf.org/html/rfc4559}.
 * <p>
 * If the authentication is successful, the original servlet request is wrapped
 * within a LikewiseHttpServletRequestWrapper object.
 * </p>
 */
public final class LikewiseNegotiateFilter implements Filter {
    
    private static final String AUTH_METHOD_NEGOTIATE = "Negotiate ";
    private static final String AUTH_METHOD_NTLM = "NTLM ";
    private static final String SAVED_USER_PRINCIPAL = "LwSavedUserPrincipal";
    private static final String SUBJECT = "javax.security.auth.subject";
    private static final String GSS_CONTEXT = "likewise-gss-context";
    private static final byte[] NTLMSSP_SIGNATURE = {0x4e, 0x54, 0x4c, 0x4d, 0x53, 0x53, 0x50, 0x00};

    private FilterConfig _config;
    private Set<String> _allowRoles;
    private Set<String> _denyRoles;
    private List<SubnetUtils> _ipAddrFilters;
    private boolean _debug;

    /**
     * Initializes the servlet filter.
     *
     * @param config configuration parameters specified for the filter.
     */
    public void init(final FilterConfig config) throws ServletException {
        this._config = config;

        this.processConfig();
    }

    /**
     * Call sequence in filter chain, where authentication is performed.
     *
     * @param request  Incoming (HTTP) servlet request.
     * @param response Outgoing (HTTP) servlet response.
     * @param chain    Next filter in chain.
     */
    public void
    doFilter(final ServletRequest request, final ServletResponse response, final FilterChain chain)
    throws IOException, ServletException
    {
        ServletRequest requestWrapper = null;

        if(this._debug) {
            this._config.getServletContext().log("Processing new request ...");
        }

        if (
            (request instanceof HttpServletRequest) &&
            (((HttpServletRequest) request).getUserPrincipal() == null) && // Previously authenticated principal
            acceptRequest((HttpServletRequest) request)
        ) {
            final HttpServletRequest httpRequest = (HttpServletRequest) request;
            final HttpServletResponse httpResponse = (HttpServletResponse) response;
            final HttpSession session = httpRequest.getSession(false);

            if(this._debug) {
                this._config.getServletContext().log("The request is accepted for session: " + ((session == null) ? null : session.getId()));
            }

            AuthResults savedResults = null;

            if(session != null) {
                savedResults = (AuthResults) session.getAttribute(LikewiseNegotiateFilter.SAVED_USER_PRINCIPAL);
            }

            if(savedResults == null) {
                 savedResults = new AuthResults();
            }

            if (!this.authenticate(httpRequest, httpResponse, savedResults)) {
                return;
            }

            if(savedResults.remoteUser == null) {
                if(this._debug) {
                    this._config.getServletContext().log("Authentication failed because savedResults.remoteUser is null");
                }

                return;
            }

            if(this._debug) {
                this._config.getServletContext().log("Authentication succeeded for user: " + savedResults.remoteUser.getName());
            }

            requestWrapper = new LikewiseHttpServletRequestWrapper(httpRequest, savedResults.remoteUser, savedResults.roles);
        }

        chain.doFilter(requestWrapper != null ? requestWrapper : request, response);
    }

    /**
     * Routine to cleanup any state.
     */
    public void destroy()
    {
        this._config = null;
    }

    /**
     * Processes parameters specified in filter configuration.
     */
    private void processConfig() {
        if (this._config != null) {
            final Enumeration<?> params = _config.getInitParameterNames();

            while (params.hasMoreElements()) {
                final String name = params.nextElement().toString();
                final String value = _config.getInitParameter(name);

                if (name.equals("allow-role")) {
                    if (this._allowRoles == null) {
                        this._allowRoles = new HashSet<String>();
                    }

                    this._allowRoles.add(value);
                }
                else {
                    if (name.equals("deny-role")) {
                        if (this._denyRoles == null) {
                            this._denyRoles = new HashSet<String>();
                        }

                        this._denyRoles.add(value);
                    }
                    else {
                        if (name.equals("remote-address-accept-filter"))
                        {
                            boolean bAdded = false;

                            if (this._ipAddrFilters == null) {
                                this._ipAddrFilters = new ArrayList<SubnetUtils>();
                            }

                            try {
                                final StringTokenizer comp = new StringTokenizer(value, ";");
                                while (comp.hasMoreElements()) {
                                   String avalue = comp.nextToken().trim();

                                   if (avalue.contains(",")) {
                                       // IP Address, Subnet Mask format
                                       final StringTokenizer tokenizer = new StringTokenizer(avalue, ", ");
                                       final String ipAddress = tokenizer.nextToken();
                                       final String subnetMask = tokenizer.hasMoreTokens() ? tokenizer.nextToken() : null;

                                       if (subnetMask != null) {
                                           this._ipAddrFilters.add(new SubnetUtils(ipAddress, subnetMask));
                                           bAdded = true;
                                       }
                                   }
                                   else {
                                       // CIDR format or plain IP Address
                                       this._ipAddrFilters.add(new SubnetUtils(avalue));
                                       bAdded = true;
                                   }
                               }
                            }
                            catch (IllegalArgumentException e) {
                            }
                            finally {
                                if (!bAdded) {
                                    this._config.getServletContext().log(
                                            "Skipping invalid remote-address-accept-filter [" + value + "]"
                                    );
                                }
                            }
                        }
                        else {
                            if (name.equalsIgnoreCase("debug")) {
                                if("true".equalsIgnoreCase(value)) {
                                    this._debug = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * Performs authentication based on the data in the incoming servlet request
     * and any session (GSS) state being preserved from earlier call.
     *
     * @param httpRequest  Incoming (HTTP) servlet request.
     * @param httpResponse Outgoing (HTTP) servlet response.
     * @param savedResults Authentication results
     * @return true if the remote principal was successfully authenticated.
     */
    private boolean
    authenticate(
        final HttpServletRequest httpRequest,
        final HttpServletResponse httpResponse,
        final AuthResults savedResults
    )
    {
        boolean authenticated = false;
        HttpSession session = null;
        boolean bKeepGssContext = false;

        try {
            if (savedResults.remoteUser != null) {

                final HttpSession ses = httpRequest.getSession(false);

                if(this._debug) {
                    this._config.getServletContext().log("Principal is not null in session: " + ((ses == null) ? null : ses.getId()));
                }

                if(this.requiresSpecialNTLMAuthorization(httpRequest)) {
                    if(this._debug) {
                        this._config.getServletContext().log("This is a POST/PUT request from IE7.");
                    }

                    if(ses != null) {
                        ses.removeAttribute(LikewiseNegotiateFilter.GSS_CONTEXT);
                        ses.removeAttribute(LikewiseNegotiateFilter.SUBJECT);
                    }
                    savedResults.remoteUser = null;
                    savedResults.roles = null;

                    return this.authenticate(httpRequest, httpResponse, savedResults);
                }

                authenticated = true;
            }
            else {
                if(this._debug) {
                    final HttpSession ses = httpRequest.getSession(false);
                    this._config.getServletContext().log("Principal is null in session: " + ((ses == null) ? null : ses.getId()));
                }

                final String authorization = httpRequest.getHeader("Authorization");

                if (authorization != null) {

                    if(this._debug) {
                        final HttpSession ses = httpRequest.getSession(false);
                        this._config.getServletContext().log("Authorization header is " + authorization + ". Session ID is " + ((ses == null) ? null : ses.getId()));
                    }

                    final Base64 base64Codec = new Base64(-1);
                    final String authType = this.getAuthType(authorization);
                    final String authContent = authorization.substring(authType.length());
                    final byte[] authContentBytes = base64Codec.decode(authContent.getBytes());

                    if(this._debug) {
                        this._config.getServletContext().log("authType is " + authType);
                        this._config.getServletContext().log("authContent is " + authContent);
                    }

                    session = httpRequest.getSession(true);

                    if(this._debug) {
                        final HttpSession ses = httpRequest.getSession(false);
                        this._config.getServletContext().log("Session ID is set to " + ((ses == null) ? null : ses.getId()));
                    }

                    GSSContext gssContext = (GSSContext) session.getAttribute(LikewiseNegotiateFilter.GSS_CONTEXT);

                    if (gssContext == null) {
                        if(this._debug) {
                            this._config.getServletContext().log("GSS context is not in the session.");
                        }

                        gssContext = new GSSContext();
                        session.setAttribute(LikewiseNegotiateFilter.GSS_CONTEXT, gssContext);
                    }

                    final GSSResult gssResult = gssContext.authenticate(authContentBytes);
                    final byte[] security_blob_out = gssResult.getSecurityBlob();

                    if (security_blob_out != null) {
                        if(this._debug) {
                            this._config.getServletContext().log("Security blob is not null.");
                        }

                        final String continueToken = new String(base64Codec.encode(security_blob_out));
                        httpResponse.addHeader("WWW-Authenticate", authType + continueToken);
                    }

                    if (gssResult.isContinueNeeded()) {
                        if(this._debug) {
                            this._config.getServletContext().log("Continuation is needed.");
                        }

                        httpResponse.setHeader("Connection", "keep-alive");
                        httpResponse.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
                        httpResponse.flushBuffer();
                        bKeepGssContext = true;
                    }
                    else {
                        if(this._debug) {
                            final HttpSession ses = httpRequest.getSession(false);
                            this._config.getServletContext().log("Authentication is complete for session " + ((ses == null) ? null : ses.getId()));
                        }

                        final String clientPrincipal = gssContext.getClientPrincipal();
                        final LikewiseUser userPrincipal = AuthenticationAdapter.findUserByName(clientPrincipal);
                        final List<String> roles = gssContext.getRoles();

                        if(this._debug) {
                            this._config.getServletContext().log("clientPrincipal is " + clientPrincipal + ". userPrincipal is " + userPrincipal.getName());
                        }

                        if (isAllowed(userPrincipal, roles)) {
                            if(this._debug) {
                                this._config.getServletContext().log("The user is allowed to access the content.");
                            }

                            Subject subject = (Subject) session.getAttribute(LikewiseNegotiateFilter.SUBJECT);

                            if (subject == null) {
                                if(this._debug) {
                                    this._config.getServletContext().log("Subject is not set in the session");
                                }

                                subject = new Subject();
                                session.setAttribute(LikewiseNegotiateFilter.SUBJECT, subject);
                            }

                            if (subject.isReadOnly()) {
                                if(this._debug) {
                                    this._config.getServletContext().log("Subject is read-only");
                                }

                                throw new ServletException("subject cannot be read-only");
                            }

                            final Set<Principal> principals = subject.getPrincipals();

                            principals.add(userPrincipal);

                            for (String role : roles) {
                                final LikewiseGroup grp = new LikewiseGroup(role);

                                if(this._debug) {
                                    this._config.getServletContext().log("Adding role " + grp.getName());
                                }

                                principals.add(grp);
                            }

                            savedResults.remoteUser = userPrincipal;
                            savedResults.roles = roles;
                            session.setAttribute(LikewiseNegotiateFilter.SAVED_USER_PRINCIPAL, savedResults);

                            authenticated = true;
                        }
                        else {
                            if(this._debug) {
                                this._config.getServletContext().log("The user is not allowed to access the content.");
                            }

                            this.sendError(httpResponse, HttpServletResponse.SC_FORBIDDEN);
                        }
                    }
                }
                else {
                    session = httpRequest.getSession(false);

                    if(this._debug) {
                        this._config.getServletContext().log("Authorization header is null in session " + ((session == null) ? null : session.getId()));
                    }

                    if (session != null) {
                        session.removeAttribute(LikewiseNegotiateFilter.GSS_CONTEXT);
                    }

                    this.sendUnauthorizedResponse(httpResponse, true);
                }
            }
        }
        catch (Exception e) {
            this._config.getServletContext().log("Authentication failed (" + this.getClass().getName() + "): " + e.getMessage());

            this.sendUnauthorizedResponse(httpResponse, false);
        }
        finally {
            if (!bKeepGssContext && (session != null)) {
                if(this._debug) {
                    this._config.getServletContext().log("Removing GSS context.");
                }

                session.removeAttribute(LikewiseNegotiateFilter.GSS_CONTEXT);
            }
        }

        return authenticated;
    }

    private boolean requiresSpecialNTLMAuthorization(final HttpServletRequest httpRequest)
    {
        boolean result = false;
        final String authorization = httpRequest.getHeader("Authorization");
        final String httpMethod = httpRequest.getMethod();

        if(
            (httpMethod != null) &&
            (httpMethod.length() != 0) &&
            (httpMethod.equals("POST") || httpMethod.equals("PUT")) &&
            (httpRequest.getContentLength() == 0) &&
            (authorization != null) &&
            (authorization.length() != 0)
        ) {
            final Base64 base64Codec = new Base64(-1);
            final String authType = this.getAuthType(authorization);
            final String authContent = authorization.substring(authType.length());
            final byte[] authContentBytes = base64Codec.decode(authContent.getBytes());

            if(this._debug) {
                this._config.getServletContext().log("In IE7 check: authType is " + authType);
                this._config.getServletContext().log("In IE7 check: authContent is " + authContent);
            }

            if (authContentBytes.length > LikewiseNegotiateFilter.NTLMSSP_SIGNATURE.length) {
                boolean signatureMatches = true;

                if(this._debug) {
                    this._config.getServletContext().log("In IE7 check: signature length is OK");
                }

                for (int i = 0; i < LikewiseNegotiateFilter.NTLMSSP_SIGNATURE.length; i++) {
                    if (authContentBytes[i] != LikewiseNegotiateFilter.NTLMSSP_SIGNATURE[i]) {
                        signatureMatches = false;
                        break;
                    }
                }

                if(this._debug) {
                    this._config.getServletContext().log("In IE7 check: signature matches");
                }

                if (signatureMatches && (authContentBytes[NTLMSSP_SIGNATURE.length] == 1)) {
                    if(this._debug) {
                        this._config.getServletContext().log("In IE7 check: this is Type1 message");
                    }

                    result = true;
                }
            }
        }

        return result;
    }

    /**
     * Checks the remote IP Address of the request for acceptance to SPNEGO
     * authentication using a configured IP Address filter. Checks the
     * "X-Forwarded-For" header for the original IP Address set by an
     * intermediate proxy server.
     *
     * @param httpRequest Incoming HTTP Servlet Request
     * @return true if the request should be accepted for SPNEGO authentication
     */
    private boolean acceptRequest(final HttpServletRequest httpRequest)
    {
        boolean bAcceptRequest = true;

        if ((this._ipAddrFilters != null) && !this._ipAddrFilters.isEmpty()) {
            String remoteIpAddr = null;
            final String forwardHdr = httpRequest.getHeader("X-Forwarded-For");

            // Whether this request was forwarded through a proxy?
            // Format of this header is as follows.
            // X-Forwarded-For: client1, proxy1, proxy2
            if ((forwardHdr != null) && !forwardHdr.trim().equals("")) {
                final StringTokenizer tokenizer = new StringTokenizer(forwardHdr, ", ");
                remoteIpAddr = tokenizer.nextToken();
            }

            if ((remoteIpAddr == null) || remoteIpAddr.trim().equals("")) {
                remoteIpAddr = httpRequest.getRemoteAddr();
            }

            bAcceptRequest = false;

            for (SubnetUtils filter : this._ipAddrFilters) {
                if (filter.getInfo().isInRange(remoteIpAddr)) {
                    bAcceptRequest = true;
                    break;
                }
            }
        }

        return bAcceptRequest;
    }

    /**
     * Determines the authentication type of the incoming http request based on
     * the authorization header.
     *
     * @param authorization Identifier from the HTTP authorization header.
     * @return supported authentication type ("NTLM" or "Negotiate").
     */
    private String getAuthType(final String authorization)
    {
        if (authorization.startsWith(LikewiseNegotiateFilter.AUTH_METHOD_NEGOTIATE)) {
            return LikewiseNegotiateFilter.AUTH_METHOD_NEGOTIATE;
        }
        else {
            if (authorization.startsWith(LikewiseNegotiateFilter.AUTH_METHOD_NTLM)) {
                return LikewiseNegotiateFilter.AUTH_METHOD_NTLM;
            }
            else {
                if(this._debug) {
                    this._config.getServletContext().log("Unsupported authentication type [" + authorization + "]");
                }

                throw new RuntimeException("Unsupported authentication type [" + authorization + "]");
            }
        }
    }

    /**
     * Builds an unauthorized HTTP response message.
     *
     * @param httpResponse Outgoing (HTTP) servlet response.
     * @param keepAlive Keep the connection alive?
     */
    private void sendUnauthorizedResponse(final HttpServletResponse httpResponse, final boolean keepAlive)
    {
        try {
            if(this._debug) {
                this._config.getServletContext().log("Sending unauthorized response. keepAlive is " + keepAlive);
            }

            httpResponse.addHeader("WWW-Authenticate", "Negotiate");
            httpResponse.addHeader("WWW-Authenticate", "NTLM");
            httpResponse.setHeader("Connection", keepAlive ? "keep-alive" : "close");
            httpResponse.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            httpResponse.flushBuffer();
        }
        catch (IOException e) {
            if(this._debug) {
                this._config.getServletContext().log("Failed to send unauthorized response: " + e.getMessage());
            }

            throw new RuntimeException(e);
        }
    }

    /**
     * Determines if the specified user principal or roles have been denied or
     * approved in that order based on the filter's configuration parameters.
     *
     * @param remoteUser remote authenticated user principal.
     * @param roles      list of roles the remote user principal is a member of.
     * @return true if the remote user principal has been approved access.
     */
    private synchronized boolean isAllowed(final LikewiseUser remoteUser, final List<String> roles)
    {
        boolean bAllowed = true;

        if (this._denyRoles != null) {
            if (this._denyRoles.contains(remoteUser.getName())) {
                bAllowed = false;
            }
            else {
                for (String role : roles) {
                    if (this._denyRoles.contains(role)) {
                        bAllowed = false;
                        break;
                    }
                }
            }
        }

        if (bAllowed && (this._allowRoles != null)) {
            bAllowed = false;

            for (String role : roles) {
                if (this._allowRoles.contains(role)) {
                    bAllowed = true;
                    break;
                }
            }
        }

        return bAllowed;
    }

    /**
     * Builds a HTTP error response.
     *
     * @param httpResponse Outgoing (HTTP) servlet response
     * @param status       HTTP error status
     */
    private void sendError(final HttpServletResponse httpResponse, final int status)
	{
		try
		{
            if(this._debug) {
                this._config.getServletContext().log("Sending error response.");
            }

			httpResponse.sendError(status);
		}
		catch(final IOException e)
		{
            if(this._debug) {
                this._config.getServletContext().log("Failed to send the error response: " + e.getMessage());
            }

			throw new RuntimeException(e);
		}
	}

    /*
    public static void main(String[] args) {
        final LikewiseNegotiateFilter filter = new LikewiseNegotiateFilter();
        boolean result = false;
        String authorization = "Negotiate TlRMTVNTUAABAAAAB4IIogAAAAAAAAAAAAAAAAAAAAAFASgKAAAADw==";
        final Base64 base64Codec = new Base64(-1);
        final String authType = filter.getAuthType(authorization);
        final String authContent = authorization.substring(authType.length());
        final byte[] authContentBytes = base64Codec.decode(authContent.getBytes());

        if (authContentBytes.length > LikewiseNegotiateFilter.NTLMSSP_SIGNATURE.length) {
            boolean signatureMatches = true;

            for (int i = 0; i < LikewiseNegotiateFilter.NTLMSSP_SIGNATURE.length; i++) {
                if (authContentBytes[i] != LikewiseNegotiateFilter.NTLMSSP_SIGNATURE[i]) {
                    signatureMatches = false;
                    break;
                }
            }

            if (signatureMatches && (authContentBytes[LikewiseNegotiateFilter.NTLMSSP_SIGNATURE.length] == 1)) {
                result = true;
            }
        }

        System.out.println("Result: " + result);
    }
    */
}

class AuthResults {
    protected LikewiseUser remoteUser;
    protected List<String> roles;
}
