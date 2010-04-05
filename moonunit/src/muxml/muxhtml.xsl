<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
   version="1.0"
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
   xmlns="http://www.w3.org/1999/xhtml">
  <xsl:output method="xml" indent="yes" encoding="UTF-8"/>
  <xsl:template match="/libraries">
    <html>
      <head>
	<title>MoonUnit Test Results</title>
	<link rel="stylesheet" type="text/css" href="muxhtml.css"/>
	<script type="text/javascript">
	  function activate_content(name)
	  {
	      var node = document.getElementById(name);
	      
	      if (node.style.display == '')
	      {
	          node.style.display = 'block';
	      }

	      if (node.style.display != 'none')
	      {
	          node.style.display = 'none';
	      }
	      else
	      {
	          node.style.display = 'block';
	      }
	  }
	  function collapse_tests()
	  {
	      var nodes = document.getElementsByName("test-content");
	      for (var i = 0; i &lt; nodes.length; i++)
              {
                  var node = nodes[i];
                  activate_content(node.id);
              }
          }
	</script>
      </head>
      <body onload="collapse_tests()">
	<div class="results">
	  <xsl:apply-templates select="library"/>
	</div>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="library">
    <div class="library">
      <div class="header">
	<div class="title"><xsl:value-of select="@name"/></div>
      </div>
      <div class="content">
	<xsl:apply-templates select="suite"/>
      </div>
    </div>
  </xsl:template>

  <xsl:template match="suite">
    <div class="suite">
      <div class="header">
	<div class="title"><xsl:value-of select="@name"/></div>
      </div>
      <div class="content">
	<xsl:apply-templates select="test"/>
      </div>
    </div>
  </xsl:template>

  <xsl:template match="test">
    <xsl:variable name="status" select="result/@status"/>
    <xsl:variable name="cid" select="generate-id()"/>
    <div class="test">
      <div class="header" onclick="activate_content('{$cid}')">
	<xsl:if test="event or normalize-space(result/.)">
	  <xsl:attribute name="full"><xsl:text>true</xsl:text></xsl:attribute>
	</xsl:if>
	<div class="result">
	  <xsl:attribute name="status">
	    <xsl:choose>
	      <xsl:when test="$status = 'pass' or $status = 'xfail'">
		<xsl:text>pass</xsl:text>
	      </xsl:when>
	      <xsl:when test="$status = 'fail' or $status = 'xpass'">
		<xsl:text>fail</xsl:text>
	      </xsl:when>
	      <xsl:when test="$status = 'skip'">
		<xsl:text>skip</xsl:text>
	      </xsl:when>
	    </xsl:choose>
	  </xsl:attribute>
	  <xsl:value-of select="$status"/>
	</div>
	<div class="title"><xsl:value-of select="@name"/></div>
      </div>
      <div class="content" id="{$cid}" name="test-content">
	<xsl:for-each select="event">
	  <div class="event">
	    <xsl:attribute name="level">
	      <xsl:value-of select="@level"/>
	    </xsl:attribute>
	    <div class="source">
	      <xsl:value-of select="@file"/>
	      <xsl:text>:</xsl:text>
	      <xsl:value-of select="@line"/>
	    </div>
	    <div class="level"><xsl:value-of select="@level"/></div>
	    <div class="message">
	      <xsl:value-of select="text()"/>
	    </div>
	  </div>
	</xsl:for-each>
	<xsl:if test="result/@file">
	  <div class="source">
	    <xsl:value-of select="result/@file"/>
	    <xsl:text>:</xsl:text>
	    <xsl:value-of select="result/@line"/>
	  </div>
	</xsl:if>
	<div class="message">
	  <xsl:value-of select="result/text()"/>
	</div>
      </div>
    </div>
  </xsl:template>
</xsl:stylesheet>
