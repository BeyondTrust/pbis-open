<?xml version="1.0"?>
<xsl:stylesheet exclude-result-prefixes="d"
                 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:d="http://docbook.org/ns/docbook"
version="1.0">

<!-- ********************************************************************
     $Id: htmltbl.xsl 8103 2008-08-04 11:37:01Z mzjn $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template match="d:colgroup" mode="htmlTable">
  <xsl:copy>
    <xsl:copy-of select="@*"/>
    <xsl:apply-templates mode="htmlTable"/>
  </xsl:copy>
</xsl:template>

<xsl:template match="d:col" mode="htmlTable">
  <xsl:copy>
    <xsl:copy-of select="@*"/>
  </xsl:copy>
</xsl:template>

<xsl:template match="d:caption" mode="htmlTable">
  <!-- do not use xsl:copy because of XHTML's needs -->
  <caption>  
    <xsl:copy-of select="@*"/>

    <xsl:apply-templates select=".." mode="object.title.markup">
      <xsl:with-param name="allow-anchors" select="1"/>
    </xsl:apply-templates>

  </caption>
</xsl:template>

<xsl:template match="d:tbody|d:thead|d:tfoot|d:tr" mode="htmlTable">
  <xsl:element name="{name(.)}">
    <xsl:copy-of select="@*"/>
    <xsl:apply-templates mode="htmlTable"/>
  </xsl:element>
</xsl:template>

<xsl:template match="d:th|d:td" mode="htmlTable">
  <xsl:element name="{name(.)}">
    <xsl:copy-of select="@*"/>
    <xsl:apply-templates/> <!-- *not* mode=htmlTable -->
  </xsl:element>
</xsl:template>

</xsl:stylesheet>
