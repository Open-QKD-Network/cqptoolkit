<!-- 
@file
@brief CQP Toolkit

@copyright Copyright (C) University of Bristol 2016
   This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
   If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
   See LICENSE file for details.
@date 04 April 2016
@author Richard Collins <richard.collins@bristol.ac.uk>

-->
<xsl:stylesheet version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- Set the resul to be plain text rather than xml -->
<xsl:output method="text" encoding="utf-8" indent="no"/>
<!-- Capture the name of the application so that it can be used at any node -->
<xsl:variable name="className" select="/interface/object[@class='GtkApplicationWindow']/@id" />
<!-- Define a constant for the header of the generated file -->
<xsl:variable name="Header">
<xsl:text>/*!
* @file
* @brief CQP Toolkit -  </xsl:text><xsl:value-of select="$className"/><xsl:text>Builder
*
* @copyright Copyright (C) University of Bristol 2016
*               See LICENSE file for details.
* @date 16 Dec 2016
* @author Richard Collins &lt;richard.collins@bristol.ac.uk&gt;
*/
#include "CQPToolkit/cqptoolkit_export.h"
#include "gtkmm.h"
#include "glibmm/refptr.h"
#include &lt;vector&gt;

namespace cqp {
    class </xsl:text><xsl:value-of select="$className"/><xsl:text>Builder {
    protected:
        using SigList = std::vector&lt;sigc::connection&gt;;

        void BlockAll(SigList&amp; cons) {
            for(auto con : cons) {
                con.block();
            }
        }

        void UnblockAll(SigList&amp; cons) {
            for(auto con : cons) {
                con.unblock();
            }
        }

</xsl:text>
</xsl:variable>

<!-- Define a constant for the footer of the generated file -->
<xsl:variable name="Footer">
<xsl:text>
    };
}</xsl:text>
</xsl:variable>

<!-- ###### Template matches ######
 These will be applied appropreatly as the tree is navigated
 Modes are only applied when a appli-template is called with the corrisponding mode value
-->
<!-- Catch all matches
    This means that if we havn't defined a template for it, nothing will be copied to the output.
    A new mode will likly need a matiching catch all
-->
<xsl:template match="text()|@*">
  <!--<xsl:value-of select="."/>-->
</xsl:template>

<xsl:template match="text()|@*" mode="builder">
  <!--<xsl:value-of select="."/>-->
</xsl:template>

<xsl:template match="text()|@*" mode="members">
  <!--<xsl:value-of select="."/>-->
</xsl:template>

<xsl:template match="text()|@*" mode="connections">
  <!--<xsl:value-of select="."/>-->
</xsl:template>

<!-- Produce member variable statements for widgets
    eg: Gtk::Entry* myEntry = nullptr;
-->
<xsl:template match="object[@id]" mode="members">
    <xsl:choose>
        <xsl:when test="@class='GtkListStore' or @class='GtkTreeViewColumn' or @class='GtkAdjustment'">
            <xsl:text>        Glib::RefPtr&lt;Gtk::</xsl:text>
            <xsl:value-of select='substring-after(@class, "Gtk")' />
            <xsl:text>&gt; </xsl:text><xsl:value-of select="@id"/><xsl:text>;&#xa;</xsl:text>
        </xsl:when>

        <xsl:otherwise>
            <xsl:text>        Gtk::</xsl:text>
            <xsl:value-of select='substring-after(@class, "Gtk")' />
            <xsl:text>* </xsl:text>
            <xsl:value-of select="@id"/>
            <xsl:text> = nullptr;&#xa;</xsl:text>
        </xsl:otherwise>
    </xsl:choose>
    <!-- Continue walking down the tree -->
    <xsl:apply-templates mode="members"/>
</xsl:template>

<!-- Produce a function to call the build and attach signals
    eg: builder->get_widget("serverAddress", serverAddress);
-->
<xsl:template match="object[@id]" mode="builder">
   <xsl:choose>
        <xsl:when test="@class='GtkListStore' or @class='GtkTreeViewColumn' or @class='GtkAdjustment'">
            <xsl:text>           </xsl:text><xsl:value-of select="@id"/><xsl:text> = RefPtr&lt;</xsl:text>
            <xsl:value-of select='substring-after(@class, "Gtk")' />
            <xsl:text>&gt;::cast_dynamic(builder->get_object(&quot;</xsl:text>
            <xsl:value-of select="@id"/><xsl:text>&quot;));</xsl:text>
        </xsl:when>

        <xsl:otherwise>
            <xsl:text>           builder->get_widget(&quot;</xsl:text>
            <xsl:value-of select="@id"/><xsl:text>&quot;, </xsl:text>
            <xsl:value-of select="@id"/><xsl:text>);</xsl:text>
        </xsl:otherwise>
   </xsl:choose>
   <xsl:text>&#xa;</xsl:text>
   <!-- Continue walking down the tree -->
   <xsl:apply-templates mode="builder"/>
</xsl:template>

<!-- this indexes all elements by their @handler attribute and is used later to produce a list of the member funtions -->
<xsl:key name="kMethodName" match="signal" use="@handler" />

<!-- Produce a list of signals which an object produces -->
<xsl:template match="object[signal]" mode="connections">
    <xsl:text>          /// </xsl:text><xsl:value-of select="@id"/><xsl:text> Connections&#xa;</xsl:text>
    <xsl:text>          struct {&#xa;</xsl:text>
    <xsl:for-each select="signal">
        <xsl:text>              /// list of attached signal handlers for </xsl:text><xsl:value-of select="@name"/><xsl:text>&#xa;</xsl:text>
        <xsl:text>              SigList </xsl:text><xsl:value-of select="translate(@name, '-', '_')"/><xsl:text>;&#xa;</xsl:text>
    </xsl:for-each>
    <xsl:text>          } </xsl:text><xsl:value-of select="@id"/><xsl:text>;&#xa;</xsl:text>
    <!-- Continue walking down the tree -->
    <xsl:apply-templates select="child" mode="connections"/>
</xsl:template>

<!-- Produce the code to connect to a signal
    eg: cx.lineAttenuation.value_changed.push_back(lineAttenuation->signal_value_changed().connect(
            sigc::mem_fun(this, &QTunnelWindow::OnLineAttenChanged)));
-->
<xsl:template match="signal" mode="builder">
    <xsl:text>           // </xsl:text>
    <xsl:value-of select="@handler"/>
    <xsl:text> -> </xsl:text>
    <xsl:value-of select="../@id"/><xsl:text>::</xsl:text><xsl:value-of select="@name"/>
    <xsl:text>&#xa;</xsl:text>

    <xsl:text>           cx.</xsl:text><xsl:value-of select="../@id"/>.<xsl:value-of select="translate(@name, '-', '_')"/><xsl:text>.push_back(</xsl:text>
    <xsl:value-of select="../@id"/>
    <!-- Type of signal
        Some signals are attached differently
    -->
    <xsl:choose>
        <xsl:when test="@name='activate'">
            <xsl:text>->property_active().signal_changed().connect(&#xa;               sigc::mem_fun(this, &amp;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>->signal_</xsl:text><xsl:value-of select="translate(@name, '-', '_')"/>
            <xsl:text>().connect(&#xa;               sigc::mem_fun(this, &amp;</xsl:text>
        </xsl:otherwise>
    </xsl:choose>

    <xsl:value-of select="$className"/>Builder::<xsl:value-of select="@handler"/>

    <!-- Type of signal
        Some signals connections have different parameters
    -->
    <xsl:choose>
        <xsl:when test="signal[@name='changed']">
            <xsl:text>), true));&#xa;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:text>)));&#xa;</xsl:text>
        </xsl:otherwise>
     </xsl:choose>
     <!-- Continue walking down the tree -->
     <xsl:apply-templates mode="builder"/>
</xsl:template>

<!-- Match the root object - should only occour once -->
<xsl:template match="/">
    <xsl:value-of select="$Header"/>
    <xsl:text>        /// @{ @name Automated widget mapping&#xa;</xsl:text>
    <xsl:text>        /// Generated by XSLT transform&#xa;</xsl:text>
    <xsl:text>        // Widget variables&#xa;</xsl:text>
    <!-- Continue walking down the tree, handling members -->
    <xsl:apply-templates mode="members"/>
<xsl:text>&#xa;</xsl:text>

    <xsl:text>        /// Connection storage&#xa;</xsl:text>
    <xsl:text>        struct {&#xa;</xsl:text>
    <!-- Continue walking down the tree hadling connections -->
    <xsl:apply-templates mode="connections"/>
    <xsl:text>        } cx;&#xa;</xsl:text>

<xsl:text>&#xa;</xsl:text>

    <!-- Use a key list of method names collected eailier to output a list of method names without duplication -->
    <xsl:text>        /// Member handlers&#xa;</xsl:text>
    <xsl:for-each select="//signal[generate-id()=generate-id(key('kMethodName',@handler)[1])]">
        <xsl:text>        /// Handler for </xsl:text><xsl:value-of select="../@id"/><xsl:text>::</xsl:text><xsl:value-of select="@name"/><xsl:text>&#xa;</xsl:text>
        <xsl:text>        virtual </xsl:text>

        <xsl:choose>
            <xsl:when test="@name='popup-menu' or @name='button-press-event'">
                <xsl:text>bool </xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>void </xsl:text>
            </xsl:otherwise>
        </xsl:choose>

        <xsl:value-of select="@handler"/><xsl:text>(</xsl:text>
        <xsl:choose>
            <xsl:when test="@name='insert-text'">
                <xsl:text>const Glib::ustring&amp; text, int* position</xsl:text>
            </xsl:when>
            <xsl:when test="@name='button-press-event'">
                <xsl:text>GdkEventButton* button_event</xsl:text>
            </xsl:when>
            <xsl:when test="@name='row-activated'">
                <xsl:text>const Gtk::TreePath&amp;, Gtk::TreeViewColumn* const&amp;</xsl:text>
            </xsl:when>
        </xsl:choose>
        <xsl:text>) = 0;&#xa;</xsl:text>
    </xsl:for-each>

<xsl:text>&#xa;</xsl:text>

    <!-- Define a helper funtion for getting the widgets and connecting signals -->
    <xsl:text>        /// Load all widgets and connect all signals&#xa;</xsl:text>
    <xsl:text>        void Build(Glib::RefPtr&lt;Gtk::Builder&gt; builder){&#xa;</xsl:text>
    <xsl:text>           using namespace Gtk;&#xa;</xsl:text>
    <xsl:text>           using namespace Glib;&#xa;</xsl:text>
    <!-- Continue walking down the tree, handling builder commands -->
    <xsl:apply-templates mode="builder"/>
    <xsl:text>        }&#xa;</xsl:text>
    <xsl:text>        /// @}&#xa;</xsl:text>

    <xsl:value-of select="$Footer"/>
</xsl:template>

</xsl:stylesheet>
