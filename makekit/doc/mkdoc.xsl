<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns="http://docbook.org/ns/docbook"
		xmlns:xi="http://www.w3.org/2001/XInclude"
		version="1.0">

  <xsl:template match="/reference">
    <reference>
      <info>
        <title><xsl:value-of select="@title"/></title>
      </info>
      <xsl:apply-templates>
	<xsl:sort select="@name"/>
      </xsl:apply-templates>
    </reference>
  </xsl:template>

  <xsl:template match="include[@format='docbook']">
    <xi:include>
      <xsl:attribute name="href">
	<xsl:value-of select="@file"/>
      </xsl:attribute>
    </xi:include>
  </xsl:template>

  <xsl:template match="function">
    <refentry version="5.0">
      <xsl:attribute name="xml:id">
	<xsl:value-of select="@name"/>
      </xsl:attribute>
      <refmeta>
	<refentrytitle><xsl:value-of select="@name"/></refentrytitle>
	<manvolnum>3mk</manvolnum>
	<refmiscinfo class="manual">MakeKit Reference</refmiscinfo>
      </refmeta>
      <refnamediv>
	<refname><xsl:value-of select="@name"/></refname>
	<refpurpose><xsl:value-of select="@brief"/></refpurpose>
      </refnamediv>
      <refsynopsisdiv>
	<title>Synopsis</title>
	<xsl:for-each select="usage">
	  <cmdsynopsis sepchar=" ">
	    <function><xsl:value-of select="../@name"/></function>
	    <xsl:apply-templates mode="params" select="param | paramsep"/>
          </cmdsynopsis>
	</xsl:for-each>
      </refsynopsisdiv>
      <xsl:if test="option">
	<refsection><info><title>Options</title></info>
	  <variablelist>
	    <xsl:for-each select="option">
	      <varlistentry>
		<xsl:choose>
		  <xsl:when test="@key">
		    <term><literal><xsl:value-of select="@key"/>=</literal><replaceable class="parameter"><xsl:value-of select="@param"/></replaceable></term>
		  </xsl:when>
		  <xsl:otherwise>
		    <term><replaceable class="parameter"><xsl:value-of select="@param"/></replaceable></term>
		  </xsl:otherwise>
		</xsl:choose>
		<listitem><para><xsl:apply-templates/></para></listitem>
	      </varlistentry>
	    </xsl:for-each>
	  </variablelist>
	</refsection>
      </xsl:if>
      <xsl:apply-templates mode="function" select="description"/>
      <xsl:apply-templates mode="function" select="example"/>
    </refentry>
  </xsl:template>
  
  <xsl:template mode="function" match="description">
    <refsection><info><title>Description</title></info>
    <xsl:apply-templates/>
    </refsection>
  </xsl:template>
  
  <xsl:template mode="function" match="example">
    <refsection><info><title>Examples</title></info>
      <programlisting>
	<xsl:apply-templates/>
      </programlisting>
    </refsection>
  </xsl:template>

  <xsl:template mode="params" match="param">
    <arg choice="plain">
      <xsl:if test="@repeat">
	<xsl:attribute name="rep">repeat</xsl:attribute>
      </xsl:if>
      <xsl:if test="@key">
	<literal><xsl:value-of select="@key"/>=</literal>
      </xsl:if>
      <replaceable class="parameter"><xsl:value-of select="."/></replaceable>
    </arg>
  </xsl:template>

  <xsl:template mode="params" match="paramsep">
    <arg choice="plain"><literal>--</literal></arg>
  </xsl:template>

  <xsl:template match="para">
    <para><xsl:apply-templates/></para>
  </xsl:template>

  <xsl:template match="param">
    <replaceable class="parameter">
      <xsl:apply-templates/>
    </replaceable>
  </xsl:template>

<xsl:template match="variable">
    <refentry version="5.0">
      <xsl:attribute name="xml:id">
	<xsl:value-of select="@name"/>
      </xsl:attribute>
      <refmeta>
	<refentrytitle><xsl:value-of select="@name"/></refentrytitle>
	<manvolnum>3mk</manvolnum>
	<refmiscinfo class="manual">MakeKit Reference</refmiscinfo>
      </refmeta>
      <refnamediv>
	<refname><xsl:value-of select="@name"/></refname>
	<refpurpose><xsl:value-of select="@brief"/></refpurpose>
      </refnamediv>
      <refsynopsisdiv>
	<title>Synopsis</title>
	<synopsis>
	  <function><xref linkend="mk_declare">mk_declare</xref></function>
	  <xsl:if test="@export">
	    <xsl:text> </xsl:text><literal>-e</literal>
	  </xsl:if>
	  <xsl:if test="@inherit">
	    <xsl:text> </xsl:text><literal>-i</literal>
	  </xsl:if>
	  <xsl:if test="@output">
	    <xsl:text> </xsl:text><literal>-o</literal>
	  </xsl:if>
	  <xsl:if test="@system">
	    <xsl:text> </xsl:text><literal>-s</literal>
	  </xsl:if>
	  <xsl:text> </xsl:text>
	  <varname><xsl:value-of select="@name"/></varname>
	</synopsis>
      </refsynopsisdiv>
      <xsl:if test="value">
	<refsection><info><title>Values</title></info>
	  <variablelist>
	    <xsl:for-each select="value">
	      <varlistentry>
		<term><literal><xsl:value-of select="@val"/></literal></term>
		<listitem><para><xsl:apply-templates/></para></listitem>
	      </varlistentry>
	    </xsl:for-each>
	  </variablelist>
	</refsection>
      </xsl:if>
      <xsl:apply-templates mode="function" select="description"/>
    </refentry>
  </xsl:template>

  <xsl:template match="var">
    <varname>
      <xsl:apply-templates/>
    </varname>
  </xsl:template>

  <xsl:template match="def">
    <varname>
      <xsl:apply-templates/>
    </varname>
  </xsl:template>

  <xsl:template match="func">
    <function>
      <xsl:apply-templates/>
    </function>
  </xsl:template>

  <xsl:template match="lit">
    <literal>
      <xsl:apply-templates/>
    </literal>
  </xsl:template>

  <xsl:template match="varname">
    <replaceable>
      <xsl:value-of select="translate(., $lowercase, $uppercase)"/>
    </replaceable>
  </xsl:template>

  <xsl:template match="cmd">
    <literal>
      <xsl:apply-templates/>
    </literal>
  </xsl:template>

  <xsl:template match="funcref">
    <function>
      <xref>
	<xsl:attribute name="linkend">
	  <xsl:value-of select="."/>
	</xsl:attribute>
	<xsl:apply-templates/>
      </xref>
    </function>
  </xsl:template>

  <xsl:template match="varref">
    <varname>
      <xref>
	<xsl:attribute name="linkend">
	  <xsl:value-of select="."/>
	</xsl:attribute>
	<xsl:apply-templates/>
      </xref>
    </varname>
  </xsl:template>

  <xsl:template match="topicref">
    <function>
      <xref>
	<xsl:attribute name="linkend">
	  <xsl:choose>
	    <xsl:when test="@ref">
	      <xsl:value-of select="@ref"/>
	    </xsl:when>
	    <xsl:otherwise>
	      <xsl:value-of select="."/>
	    </xsl:otherwise>
	  </xsl:choose>
	</xsl:attribute>
	<xsl:apply-templates/>
      </xref>
    </function>
  </xsl:template>
  <xsl:variable name="lowercase" select="'abcdefghijklmnopqrstuvwxyz'"/>
  <xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>
</xsl:stylesheet>
