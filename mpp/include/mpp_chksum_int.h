function MPP_CHKSUM_INT_( var, pelist, mask_val )
  integer(LONG_KIND) :: MPP_CHKSUM_INT_
      MPP_TYPE_, intent(in) :: var MPP_RANK_
      integer, optional :: pelist(:)
  MPP_TYPE_, intent(in), optional :: mask_val

  if ( PRESENT(mask_val) ) then
     !PACK on var/=mask_val ignores values in var
     !equiv to setting those values=0, but on sparse arrays
     !pack should return much smaller array to sum
     MPP_CHKSUM_INT_ = sum( INT( PACK(var,var/=mask_val),LONG_KIND) )
  else
     MPP_CHKSUM_INT_ = sum(INT(var,LONG_KIND))
  end if

      call mpp_sum( MPP_CHKSUM_INT_, pelist )
      return

    end function MPP_CHKSUM_INT_


!Handles real mask for easier implimentation
! until exists full integer vartypes...
function MPP_CHKSUM_INT_RMASK_( var, pelist, mask_val )
  integer(LONG_KIND) :: MPP_CHKSUM_INT_RMASK_
  MPP_TYPE_, intent(in) :: var MPP_RANK_
  integer, optional :: pelist(:)
  real, intent(in) :: mask_val
  integer(KIND(var))::imask_val
  integer(KIND(var))::itmpVarP(2)
  real(KIND(var))::rtmpVarP(2)
  !high fidelity error message
  character(LEN=1) :: tmpStr1,tmpStr2,tmpStr3
  character(LEN=32) :: tmpStr4,tmpStr5
  character(LEN=512) :: errStr

! Primary Logic: These first two are the "expected" branches.
!! These all resolve to MPP_FILL_INT
  !!Should catch real "default_fill"(MPP_FILL_DOUBLE), .OR. mask_val == MPP_FILL_FLOAT
  if (mask_val == MPP_FILL_DOUBLE ) then !this is FMS variable default fill, and is the crutch supporting nonregistered...
     ! we've packed an MPP_FILL_
     imask_val = MPP_FILL_INT
  !! Should catch alternative storage representations
  !!! Current NETCDF fill values (AKA MPP_FILL_*) designed towards CEILING(MPP_FILL_{FLOAT,DOUBLE},kind=4byte)=MPP_FILL_INT
  else if ( CEILING(mask_val,INT_KIND) == MPP_FILL_INT ) then
     ! we've packed an MPP_FILL_
     imask_val = MPP_FILL_INT
! Secondary Logic: First Class Corner Cases
!! These all resolve to MPP_FILL_INT
  !! Same KIND, works when parity between *_KINDS across REAL and INTEGER
  else if ( KIND(mask_val) == KIND(var) ) then
     ! see if previously ducktyped integer via transfer
     itmpVarP(1) = TRANSFER(mask_val , itmpVarP(1))
     if ( itmpVarP(1) == MPP_FILL_INT ) then
        ! we've packed an MPP_FILL_
        imask_val = MPP_FILL_INT
     end if
  !! r8 mask, i4 var
  else if ( KIND(mask_val)==DOUBLE_KIND .AND. KIND(var)==INT_KIND ) then
     itmpVarP = TRANSFER(mask_val, itmpVarP)
     ! see if previously ducktyped integer via transfer into either long word
     ! .OR. if packed the double as a long long (two words), future MPP_FILL_LONG when NETCDF pushes int64_t
     if ( ANY(itmpVarP == MPP_FILL_INT ) .OR. ANY( itmpVarP == CEILING(MPP_FILL_DOUBLE,LONG_KIND) ) ) then
        ! we've packed an MPP_FILL_
        imask_val = MPP_FILL_INT
     else ! see if previously packed float via transfer into either long word
        rtmpVarP = TRANSFER(mask_val, rtmpVarP)
        if ( ANY(rtmpVarP == MPP_FILL_DOUBLE ) .OR. ANY(CEILING(rtmpVarP,INT_KIND) == MPP_FILL_INT) ) then
           ! we've packed an MPP_FILL_
           imask_val = MPP_FILL_INT
        end if
     end if
! according to the Walpiri, there should be no third logic, it represent too many, ie low class corner cases..
! so here be dragons, we should not be doing either of these things, but someone will try,
! and we are going to help them by giving detail
  !! r4 mask, i8 var
  else if ( KIND(mask_val)==FLOAT_KIND .AND. KIND(var)==LONG_KIND ) then
     !!!MPP_FILL_ would have been caught in the Primary Logic tests,
     ! construct detailed errStr
     errStr = "mpp_chksum: mpp_chksum_i"
     write(unit=tmpStr1,fmt="(I1)") KIND(var)
     write(unit=tmpStr2,fmt="(I1)") SIZE(SHAPE(var))
     errStr = errStr // tmpStr1 // "_" // tmpStr2 // "d_rmask passed int var with REAL("
     write(unit=tmpStr3,fmt="(I1)") KIND(mask_val)
     errStr = errStr // tmpStr3 // ") mask_val="
     write(unit=tmpStr4,fmt=*) mask_val

     imask_val = CEILING(mask_val,LONG_KIND)

     write(unit=tmpStr5,fmt=*) imask_val
     errStr = errStr // trim(tmpStr4) // "has been called in a strange way; supported MPP_FILL_ not found." //&
          "Check your KINDS, _FillValue, pack and mask_val for correctness. "// &
           "MPP will continue using CEILING(mask_val,LONG_KING)="// trim(tmpStr5) // &
           ".Hint: Using defaults, or explicit MPP_FILL_{INT,FLOAT,DOUBLE} instead of custom values should avoid this." //&
           "THIS WILL BE FATAL IN THE FUTURE!"
     call mpp_error(WARNING, trim(errStr) )

  else
     ! construct detailed errStr
      errStr = "mpp_chksum: mpp_chksum_i"
      write(unit=tmpStr1,fmt="(I1)") KIND(var)
      write(unit=tmpstr2,fmt="(I1)") SIZE(SHAPE(var))
      errStr = errStr // tmpStr1 // "_" // tmpstr2 // "d_rmask passed int var with REAL("
      write(unit=tmpstr3,fmt="(I1)") KIND(mask_val)
      errStr = errStr // tmpstr3 // ") mask_val="
      write(unit=tmpstr4,fmt=*) mask_val
      errStr = errStr // trim(tmpstr4) // "has been called with these strange values. Check your KINDS, _FillValue, pack and mask_val. // &
           Hint: Try being explicit and using MPP_FILL_{INT,FLOAT,DOUBLE}. Continuing by using the default MPP_FILL_INT. // &
           THIS WILL BE FATAL IN THE FUTURE!"
      call mpp_error(WARNING, trim(errStr) )

      imask_val = MPP_FILL_INT
   end if

  MPP_CHKSUM_INT_RMASK_ = mpp_chksum(var,pelist,mask_val=imask_val)

  return

end function MPP_CHKSUM_INT_RMASK_
