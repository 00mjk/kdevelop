/*********************************************************
 * cclasspropertiesdlgimpl.h                                                                  *
 * Safe Implementation class from cclasspropertiesdlg.ui                            *
 *------------------------------------------------------------------------*
 * Author :                                                                                         *
 *            Serge Lussier                                                                      *
 * email: serge.lussier@videotron.ca                                                      *
 * Started on January 15, 2001                                                            *
 *-----------------------------------------------------------------------*
 * NOTE:                                                                                       *
 * Before re-implement the ui file (with '-impl' option) , make a safe copy of             *
 * this file then re-put                                                                                    *
 * added stuff from this file to the new file.                                                        *
 ***************************************************************/


#ifndef CCLASSPROPERTIESDLGIMPL_H
#define CCLASSPROPERTIESDLGIMPL_H
#include "wzconnectdlg.h"
//#include "./classparser/ClassStore.h"
#include "./classparser/ClassParser.h"


/** For what action the dialog was called and current action : */
enum CTPACTION {CTPVIEW=0, CTPADDATTR, CTPADDMETH, CTPADDSIGNAL, CTPADDSLOT, CTPCONNECTSIG };

// Index of tabs
#define CTPCLASSVIEW    (int)0
#define CTPATTRIBUTE     (int)1
#define CTPMETHOD         (int)2
#define CTPSIGNAL          (int)3
#define CTPSLOT             (int)4
#define CTPIMPL             (int)5
//-------------------------------

#include <kconfig.h>

class CClassToolDlg;

class CClassPropertiesDlgImpl : public CClassPropertiesDlg
{ 
    Q_OBJECT

public:
        CClassPropertiesDlgImpl( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
        CClassPropertiesDlgImpl( CTPACTION action, CClassToolDlg* ctdlg, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~CClassPropertiesDlgImpl();
  /**  */
  void setCurrentClassName ( const QString& );
  /** Fill'in data into the widgets depending on which TAB ' vtab ' is
       invoqued and the current action.*/
  void updateData( int vtab );
  /** Check if data onscreen is the same as its CTabData.
    @return true if one item is different.
 */
  bool isModified( int tabw );
  /**  */
  void setImplTabMethList ( QList<CParsedMethod> *MethList, bool bclear=true);
  /**  */
  void setSlotTabSlotList ( QList<CParsedMethod> *MethList , bool bmatchsig=false);
  /**  */
  void setSigTabAttrList ( QList<CParsedAttribute> *AttrList );
  /**  */
  void setClass ( CParsedClass* aClass );;
  void setStore ( CClassStore* s) { store = s; }
  void init();
  /** This function tries guess if aName is a QT or a KDE class then set filename of the include file according to the classname.
 */
  CParsedClass* unParsedClass( const QString& );
  /**  */
  void viewParents();
  /**  */
  void viewChildren();
  /**  */
  void setClassToolDlg( CClassToolDlg* ct);
public slots:
    void slotBtnApply();
    void slotBtnUndo();
    void slotSigClassNameEditEnter();
  /**  */
  void slotClassViewChanged( CParsedClass* );

protected slots:
    void slotMethVirtualState(int);
    void slotSlotModifierChanged(int );
    void slotAddSlotState(int);
    void slotImplMethodSelected(const QString&);
    void slotMethAccessChanged(int);
    void slotMethModifierChanged(int);
    void slotMethNameChanged( const QString & );
    void slotMethTypeChanged( const QString & );
    void slotSigAccessChanged(int);
    void slotSigAddSignalState(int);
    void slotSigMemberSelected(const QString&);
    void slotSigNameChanged( const QString& );
    void slotSigSignalSelected(const QString&);
    void slotSlotAccessChanged(int);
    void slotSlotMemberSelected(const QString&);
    void slotSlotNameChanged( const QString& );
    void slotVarNameChanged( const QString& );
    void slotVarTypeChanged( const QString& );
    void slotTabChanged( QWidget* );
  /**  */
  //void slotSetClass( CParsedClass* );
    protected:
    virtual void resizeEvent( QResizeEvent* e) ;
  /**  */
  void getClassNameFromString( const  QString & aName, QString & newName);
  /**  */
  void setSignalsMemberList( CParsedClass* );
    class CTabData {
        public:
            QString editFields[2];
            QString combos[2];
            bool Access[4];
            bool Modifier[3];
            bool bModified;
            bool bApplied;
    };
    CTabData tbdata[6];
    CTPACTION ctpAction;
    /** Table of pointer to tabWidgets to use for iteration. internal use.*/
    QWidget* WidgetTable[6];
    CParsedClass *currentClass;
    /**  */
    CParsedArgument* Arg;
    /**  */
    CParsedMethod* slotMethod;
    /** current signal member */
    CParsedMethod* signalMethod;
    /**  */
    CParsedMethod* implMethod;
    CClassStore* store;

private: // Private attributes
  /**  */
  CParsedClass* classOfSig;
  KConfig* config;
  /**  */
  CClassParser* theParser;
  /** Holds the class store of the selected Working class variable member */
  CParsedClass* selectedSigAttrClass;
  /** used to build the connect instruction.
        If value is 0 ( NULL ) then the default class member [ this ]  is used*/
  CParsedAttribute* attrMember;
  CClassToolDlg* CTDlg;
  void applyAddAttribute();
  void applyAddMethod();
  void applySignalSlotMapImplementation();
  /**  */
  void getMemberFromString ( const QString& str, QString& newName);
signals: // Signals
  /**  */
  void sigAddMethod( const char *, CParsedMethod*);
  void sigAddAttribute( const char*, CParsedAttribute*);
  void sigSigSlotMapImplement ( CParsedClass*, const QString&, CParsedMethod* );
};

#endif // CCLASSPROPERTIESDLGIMPL_H
