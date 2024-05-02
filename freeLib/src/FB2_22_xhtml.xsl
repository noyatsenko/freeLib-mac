<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:fb="http://www.gribuser.ru/xml/fictionbook/2.0">
	<xsl:output method="xml" encoding="UTF-8"/>
	<xsl:key name="note-link" match="fb:section" use="@id"/>
	<xsl:template match="/*">
		<html>
			<head>
				<xsl:if test="fb:description/fb:title-info/fb:lang = 'ru'">
					<meta HTTP-EQUIV="content-type" CONTENT="text/html; charset=UTF-8"/>
				</xsl:if>
				<title>
					<xsl:value-of select="fb:description/fb:title-info/fb:book-title"/>
				</title>
				<style type="text/x-oeb1-css">
					A { color : #0002CC }
					A:HOVER { color : #BF0000 }
					BODY { background-color : #FEFEFE; color : #000000; font-family : Verdana, Geneva, Arial, Helvetica, sans-serif; text-align : justify }
					H1{ font-size : 160%; font-style : normal; font-weight : bold; text-align : left; border : 1px solid Black;  background-color : #E7E7E7; margin-left : 0px;  page-break-before : always; }
					H2{ font-size : 130%; font-style : normal; font-weight : bold; text-align : left; background-color : #EEEEEE;  border : 1px solid Gray;  page-break-before : always; }
					H3{ font-size : 110%; font-style : normal; font-weight : bold; text-align : left;  background-color : #F1F1F1;  border : 1px solid Silver;}
					H4{ font-size : 100%; font-style : normal; font-weight : bold; text-align : left; border : 1px solid Gray;  background-color : #F4F4F4;}
					H5{ font-size : 100%; font-style : italic; font-weight : bold; text-align : left; border : 1px solid Gray;  background-color : #F4F4F4;}
					H6{ font-size : 100%; font-style : italic; font-weight : normal; text-align : left; border : 1px solid Gray;  background-color : #F4F4F4;}
					SMALL{ font-size : 80% }
					BLOCKQUOTE{ margin-left :4em; margin-top:1em; margin-right:0.2em;}
					HR{ color : Black }
					UL{margin-left: 0}
					.epigraph{width:50%; margin-left : 35%;}
				</style>
			</head>
			<body>


						<h4 align="center">
        					<xsl:value-of select="fb:description/fb:title-info/fb:book-title"/>
						</h4>

<xsl:for-each select="fb:description/fb:title-info/fb:coverpage/fb:image">
		<xsl:call-template name="image"/>
		</xsl:for-each>

				<xsl:for-each select="fb:description/fb:title-info/fb:annotation">

					<div>
						<xsl:call-template name="annotation"/>
					</div>
					<hr/>
				</xsl:for-each>
				<!-- BUILD TOC -->
				<ul>
					<xsl:apply-templates select="fb:body" mode="toc"/>
				</ul>
				<hr/>
				<!-- BUILD BOOK -->
				<xsl:for-each select="fb:body">
					<xsl:if test="position()!=1">
						<hr/>
					</xsl:if>
					<xsl:if test="@name">
						<h4 align="center">
							<xsl:value-of select="@name"/>
						</h4>
					</xsl:if>
					<!-- <xsl:apply-templates /> -->
					<xsl:apply-templates/>
				</xsl:for-each>
			</body>
		</html>
	</xsl:template>
	<!-- author template -->
	<xsl:template name="author">
		<xsl:value-of select="fb:first-name"/>
		<xsl:text disable-output-escaping="no">&#032;</xsl:text>
		<xsl:value-of select="fb:middle-name"/>&#032;
         <xsl:text disable-output-escaping="no">&#032;</xsl:text>
		<xsl:value-of select="fb:last-name"/>
		<br/>
	</xsl:template>
	<!-- secuence template -->
	<xsl:template name="sequence">
		<LI/>
		<xsl:value-of select="@name"/>
		<xsl:if test="@number">
			<xsl:text disable-output-escaping="no">,&#032;#</xsl:text>
			<xsl:value-of select="@number"/>
		</xsl:if>
		<xsl:if test="fb:sequence">
			<ul>
				<xsl:for-each select="fb:sequence">
					<xsl:call-template name="sequence"/>
				</xsl:for-each>
			</ul>
		</xsl:if>
		<!--      <br/> -->
	</xsl:template>
	<!-- toc template -->
	<xsl:template match="fb:section|fb:body" mode="toc">
		<xsl:choose>
			<xsl:when test="name()='body' and position()=1 and not(fb:title)">
				<xsl:apply-templates select="fb:section" mode="toc"/>
			</xsl:when>
			<xsl:otherwise>
				<li>
					<a href="#TOC_{generate-id()}"><xsl:value-of select="normalize-space(fb:title/fb:p[1] | @name)"/></a>
					<xsl:if test="fb:section">
						<ul><xsl:apply-templates select="fb:section" mode="toc"/></ul>
					</xsl:if>
				</li>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>
	<!-- description -->
	<xsl:template match="fb:description">
		<xsl:apply-templates/>
	</xsl:template>
	<!-- body -->
	<xsl:template match="fb:body">
		<div><xsl:apply-templates/></div>
	</xsl:template>

	<xsl:template match="fb:section">
		<a name="TOC_{generate-id()}"></a>
		<xsl:if test="@id">
			<xsl:element name="a">
				<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
			</xsl:element>
		</xsl:if>
		<xsl:apply-templates/>
	</xsl:template>
	
	
	<!-- section/title -->
	<xsl:template match="fb:section/fb:title|fb:poem/fb:title">
		<xsl:choose>
			<xsl:when test="count(ancestor::node()) &lt; 9">
				<xsl:element name="{concat('h',count(ancestor::node())-3)}">
					<a name="TOC_{generate-id()}"></a>
					<xsl:if test="@id">
						<xsl:element name="a">
							<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
						</xsl:element>
					</xsl:if>
					<xsl:apply-templates/>
				</xsl:element>
			</xsl:when>
			<xsl:otherwise>
				<xsl:element name="h6">
					<xsl:if test="@id">
						<xsl:element name="a">
							<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
						</xsl:element>
					</xsl:if>
					<xsl:apply-templates/>
				</xsl:element>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>
	<!-- section/title -->
	<xsl:template match="fb:body/fb:title">
		<h1><xsl:apply-templates mode="title"/></h1>
	</xsl:template>

	<xsl:template match="fb:title/fb:p">
		<xsl:apply-templates/><xsl:text disable-output-escaping="no">&#032;</xsl:text><br/>
	</xsl:template>
	<!-- subtitle -->
	<xsl:template match="fb:subtitle">
		<xsl:if test="@id">
			<xsl:element name="a">
				<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
			</xsl:element>
		</xsl:if>
		<h5>
			<xsl:apply-templates/>
		</h5>
	</xsl:template>
	<!-- p -->
	<xsl:template match="fb:p">
		<div align="justify"><xsl:if test="@id">
				<xsl:element name="a">
					<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
				</xsl:element>
			</xsl:if>	&#160;&#160;&#160;<xsl:apply-templates/></div>
	</xsl:template>
	<!-- strong -->
	<xsl:template match="fb:strong">
		<b><xsl:apply-templates/></b>
	</xsl:template>
	<!-- emphasis -->
	<xsl:template match="fb:emphasis">
		<i>	<xsl:apply-templates/></i>
	</xsl:template>
	<!-- style -->
	<xsl:template match="fb:style">
		<span class="{@name}"><xsl:apply-templates/></span>
	</xsl:template>
	<!-- empty-line -->
	<xsl:template match="fb:empty-line">
		<br/>
	</xsl:template>
	<!-- link -->
	<xsl:template match="fb:a">
		<xsl:element name="a">
			<xsl:attribute name="href"><xsl:value-of select="@xlink:href"/></xsl:attribute>
			<xsl:attribute name="title">
				<xsl:choose>
					<xsl:when test="starts-with(@xlink:href,'#')"><xsl:value-of select="key('note-link',substring-after(@xlink:href,'#'))/fb:p"/></xsl:when>
					<xsl:otherwise><xsl:value-of select="key('note-link',@xlink:href)/fb:p"/></xsl:otherwise>
				</xsl:choose>
			</xsl:attribute>
			<xsl:choose>
				<xsl:when test="(@type) = 'note'">
					<sup>
						<xsl:apply-templates/>
					</sup>
				</xsl:when>
				<xsl:otherwise>
					<xsl:apply-templates/>
				</xsl:otherwise>
			</xsl:choose>
		</xsl:element>
	</xsl:template>
	<!-- annotation -->
	<xsl:template name="annotation">
		<xsl:if test="@id">
			<xsl:element name="a">
				<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
			</xsl:element>
		</xsl:if>
		<h3>Annotation</h3>
		<xsl:apply-templates/>
	</xsl:template>
	<!-- epigraph -->
	<xsl:template match="fb:epigraph">
		<blockquote class="epigraph">
			<xsl:if test="@id">
				<xsl:element name="a">
					<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
				</xsl:element>
			</xsl:if>
			<xsl:apply-templates/>
		</blockquote>
	</xsl:template>
	<!-- epigraph/text-author -->
	<xsl:template match="fb:epigraph/fb:text-author">
		<blockquote>
			<i><xsl:apply-templates/></i>
		</blockquote>
	</xsl:template>
	<!-- cite -->
	<xsl:template match="fb:cite">
		<blockquote>
		<xsl:if test="@id">
			<xsl:element name="a">
				<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
			</xsl:element>
		</xsl:if>
		<xsl:apply-templates/>
		</blockquote>
	</xsl:template>
	<!-- cite/text-author -->
	<xsl:template match="fb:text-author">
		<blockquote>
		<i>	<xsl:apply-templates/></i></blockquote>
	</xsl:template>
	<!-- date -->
	<xsl:template match="fb:date">
		<xsl:choose>
			<xsl:when test="not(@value)">
				&#160;&#160;&#160;<xsl:apply-templates/>
				<br/>
			</xsl:when>
			<xsl:otherwise>
				&#160;&#160;&#160;<xsl:value-of select="@value"/>
				<br/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>
	<!-- poem -->
	<xsl:template match="fb:poem">
		<blockquote>
			<xsl:if test="@id">
				<xsl:element name="a">
					<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
				</xsl:element>
			</xsl:if>
			<xsl:apply-templates/>
		</blockquote>
	</xsl:template>

	<!-- stanza -->
	<xsl:template match="fb:stanza">
		<xsl:apply-templates/>
		<br/>
	</xsl:template>
	<!-- v -->
	<xsl:template match="fb:v">
		<xsl:if test="@id">
			<xsl:element name="a">
				<xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute>
			</xsl:element>
		</xsl:if>
		<xsl:apply-templates/><br/>
	</xsl:template>
	<!-- image -->
	<xsl:template match="fb:image" name="image">
		<div align="center">
			<img border="1">
				<xsl:choose>
					<xsl:when test="starts-with(@xlink:href,'#')">
						<xsl:attribute name="src"><xsl:text>data:</xsl:text><xsl:variable name="href" select="substring-after(@xlink:href,'#')" /><set variable="href" expression="substring-after(@xlink:href,'#')"/><xsl:value-of select="//fb:binary[@id=$href]/@content-type" disable-output-escaping="yes" /><xsl:text>;base64,</xsl:text><!--<xsl:value-of select="substring-after(@xlink:href,'#')"/>--><xsl:value-of select="//fb:binary[@id=$href]"  disable-output-escaping="yes"/></xsl:attribute>

					</xsl:when>
					<xsl:otherwise>
						<xsl:attribute name="src"><xsl:value-of select="@xlink:href"/></xsl:attribute>
					</xsl:otherwise>
				</xsl:choose>
			</img>
		</div>
	</xsl:template>
</xsl:stylesheet>
