<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text"/>
  <xsl:variable name="timestamp" select="/coverage/@timestamp" />
  <xsl:param name="buildid" select="'-1'" />
  <xsl:template match="coverage">filename, branch-rate, line-rate, timestamp, build_id
    "all",  <xsl:value-of select="@branch-rate" />, <xsl:value-of select="@line-rate" />, <xsl:value-of select="$timestamp"/>, <xsl:value-of select="$buildid"/> <xsl:text>&#xa;</xsl:text>
    <xsl:for-each select="//class">"<xsl:value-of select="@filename" />",  <xsl:value-of select="@branch-rate" />, <xsl:value-of select="@line-rate" />, <xsl:value-of select="$timestamp"/>, <xsl:value-of select="$buildid"/> <xsl:text>&#xa;</xsl:text></xsl:for-each>
  </xsl:template>
</xsl:stylesheet>
